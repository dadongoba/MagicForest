#include "ForestRoadAIController.h"
#include "ForestCrouchNavigation.h"
#include "ForestNPCInteraction.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavAreas/NavArea_Null.h"
#include "NavModifierComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
DEFINE_LOG_CATEGORY_STATIC(LogForestRoadAI,Log,All);
AForestRoadAIController::AForestRoadAIController(const FObjectInitializer& Init)
 :Super(Init.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{PrimaryActorTick.bCanEverTick=true;DefaultNavigationFilterClass=UForestRoadQueryFilter::StaticClass();}
void AForestRoadAIController::OnPossess(APawn* InPawn)
{
 Super::OnPossess(InPawn);
 if(auto* C=Cast<ACharacter>(InPawn))
 {
  // Keep physical grounding and large obstacle collision, but use an NPC
  // object channel so scenery can let NPCs through independently of players.
  C->GetCapsuleComponent()->SetCollisionProfileName(TEXT("ForestNPCPawn"));
  TArray<USkeletalMeshComponent*> Meshes;C->GetComponents<USkeletalMeshComponent>(Meshes);
  for(auto* Mesh:Meshes)
  {
   Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   // Restore the authored locomotion: procedural crouch bone rotations were
   // deforming the character during low-clearance transitions.
   Mesh->SetDisablePostProcessBlueprint(true);
  }
  auto* M=C->GetCharacterMovement();M->NavAgentProps.bCanCrouch=true;M->SetCrouchedHalfHeight(60);
  M->MaxWalkSpeedCrouched=100;M->SetUpdateNavAgentWithOwnersCollisions(false);
  M->NavAgentProps.AgentRadius=42;M->NavAgentProps.AgentHeight=192;
  if(!C->FindComponentByClass<UForestAutoCrouchComponent>())
  {auto* Component=NewObject<UForestAutoCrouchComponent>(C,TEXT("ForestAutoCrouch"));C->AddInstanceComponent(Component);Component->RegisterComponent();}
 }
 if(auto* Crowd=Cast<UCrowdFollowingComponent>(GetPathFollowingComponent()))
 {Crowd->SetCrowdSeparation(true);Crowd->SetCrowdSeparationWeight(2);Crowd->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);}
 RebuildRoadNetwork();NextDecision=GetWorld()->GetTimeSeconds()+FMath::FRandRange(0.3f,1.f);
 if(!UGameplayStatics::GetActorOfClass(this,AForestNPCInteraction::StaticClass()))GetWorld()->SpawnActor<AForestNPCInteraction>();
}
void AForestRoadAIController::OnUnPossess()
{EndInteraction();CancelPlayerApproach();bSuppressCompletion=true;StopMovement();Request=FAIRequestID::InvalidRequest;Super::OnUnPossess();bSuppressCompletion=false;}
void AForestRoadAIController::DisableLegacyWanderTimer()
{
 if(GetPawn() && GetPawn()->FindFunction(TEXT("ChooseNextWanderTarget_0")))
 {FTimerDynamicDelegate D;D.BindUFunction(GetPawn(),TEXT("ChooseNextWanderTarget_0"));auto Handle=GetWorldTimerManager().K2_FindDynamicTimerHandle(D);GetWorldTimerManager().ClearTimer(Handle);}
}
void AForestRoadAIController::RebuildRoadNetwork()
{RoadPoints.Reset();for(TActorIterator<AForestRoadPreference> It(GetWorld());It;++It)if(!It->ActorHasTag(TEXT("NPC_RuntimeAvoid")))RoadPoints.Add(It->GetActorLocation()-FVector(0,0,70));}
bool AForestRoadAIController::ResolvePoint(const FVector& Desired,FVector& OutPoint,float& OutLength) const
{
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());if(!Nav || !GetPawn())return false;
 const auto* Data=Nav->GetNavDataForProps(GetNavAgentPropertiesRef(),GetPawn()->GetActorLocation());if(!Data)return false;
 const auto Filter=UNavigationQueryFilter::GetQueryFilter(*Data,this,DefaultNavigationFilterClass);
 FNavLocation Projected;if(!Nav->ProjectPointToNavigation(Desired,Projected,FVector(140,140,100),Data,Filter))return false;
 FPathFindingQuery Query(this,*Data,GetPawn()->GetNavAgentLocation(),Projected.Location,Filter);Query.SetAllowPartialPaths(false);
 const auto Result=Nav->FindPathSync(GetNavAgentPropertiesRef(),Query);
 if(!Result.IsSuccessful() || !Result.Path.IsValid() || Result.Path->IsPartial())return false;
 OutPoint=Projected.Location;OutLength=Result.Path->GetLength();return OutLength>250;
}
bool AForestRoadAIController::IsRecentlyUsed(const FVector& Point) const
{
 for(const FVector& Recent:RecentGoals)if(FVector::Dist2D(Recent,Point)<300)return true;
 const double Now=GetWorld()->GetTimeSeconds();for(const auto& Bad:AvoidedPoints)if(Bad.Until>Now && FVector::Dist2D(Bad.Location,Point)<350)return true;return false;
}
void AForestRoadAIController::StartRoaming()
{
 if(!GetPawn() || bInteracting)return;
 const double Now=GetWorld()->GetTimeSeconds();AvoidedPoints.RemoveAll([Now](const FCooldownPoint& P){return P.Until<=Now;});
 TArray<FVector> Candidates;const FVector Here=GetPawn()->GetActorLocation();
 for(const FVector& P:RoadPoints){const float D=FVector::Dist2D(Here,P);if(D>=MinRoamDistance && D<=MaxRoamDistance && !IsRecentlyUsed(P))Candidates.Add(P);}
 for(int32 I=Candidates.Num()-1;I>0;--I)Candidates.Swap(I,FMath::RandRange(0,I));
 bool Found=false;float Length=0;
 for(int32 I=0;I<FMath::Min(30,Candidates.Num());++I)if(ResolvePoint(Candidates[I],ApproachPoint,Length) && Length<=MaxRoamDistance*2.5f){Found=true;break;}
 if(!Found)
 {
  auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());auto* Data=Nav?Nav->GetNavDataForProps(GetNavAgentPropertiesRef(),Here):nullptr;
  if(Data)
  {
   const auto Filter=UNavigationQueryFilter::GetQueryFilter(*Data,this,DefaultNavigationFilterClass);
   for(int32 I=0;I<12 && !Found;++I){FNavLocation Random;if(Nav->GetRandomReachablePointInRadius(GetPawn()->GetNavAgentLocation(),1200,Random,Data,Filter) && FVector::Dist2D(Here,Random.Location)>300 && !IsRecentlyUsed(Random.Location))Found=ResolvePoint(Random.Location,ApproachPoint,Length);}
  }
 }
 if(!Found){StatusNote=TEXT("Searching for another reachable road");NextDecision=Now+1;RecentGoals.Reset();return;}
 FAIMoveRequest Move;Move.SetGoalLocation(ApproachPoint);Move.SetAcceptanceRadius(60);
 Move.SetReachTestIncludesAgentRadius(false);Move.SetReachTestIncludesGoalRadius(false);Move.SetAllowPartialPath(false);Move.SetNavigationFilter(DefaultNavigationFilterClass);Move.SetUsePathfinding(true);
 const auto Result=MoveTo(Move);Request=Result.MoveId;ProgressLocation=Here;ProgressTime=Now;
 if(Result.Code==EPathFollowingRequestResult::AlreadyAtGoal)CompleteWalk();else if(Result.Code==EPathFollowingRequestResult::Failed)HandleRouteFailure(TEXT("Move request rejected"));
 else{StatusNote=TEXT("Road roaming");UE_LOG(LogForestRoadAI,Log,TEXT("%s: ROAM to %s, path %.0fcm"),*GetNameSafe(GetPawn()),*ApproachPoint.ToString(),Length);}
}
void AForestRoadAIController::HandleRouteFailure(const FString& Reason)
{
 const double Now=GetWorld()->GetTimeSeconds();++RouteFailures;StatusNote=Reason;Request=FAIRequestID::InvalidRequest;AvoidedPoints.Add({ApproachPoint,Now+AvoidDuration});
 if(GetPawn())
 {
  const FVector Here=GetPawn()->GetActorLocation();const FVector Heading=(ApproachPoint-Here).GetSafeNormal2D();
  if(!Heading.IsNearlyZero())
  {
   FActorSpawnParameters Params;Params.ObjectFlags|=RF_Transient;
   auto* Avoid=GetWorld()->SpawnActor<AForestRoadPreference>(Here+Heading*220,FRotator::ZeroRotator,Params);
   if(Avoid){Avoid->Tags.Add(TEXT("NPC_RuntimeAvoid"));Avoid->Extent=FVector(100,100,180);Avoid->OnConstruction(Avoid->GetActorTransform());Avoid->Modifier->SetAreaClass(UNavArea_Null::StaticClass());Avoid->SetLifeSpan(AvoidDuration);}
  }
  for(int32 I=MovementHistory.Num()-1;I>=0;--I)if(FVector::Dist2D(Here,MovementHistory[I])>100){RecoveryPoint=MovementHistory[I];RecoveryUntil=Now+1.5;break;}
 }
 NextDecision=Now+0.4;UE_LOG(LogForestRoadAI,Warning,TEXT("%s: avoid failed route and keep roaming: %s"),*GetNameSafe(GetPawn()),*Reason);
}
void AForestRoadAIController::CompleteWalk()
{
 if(!GetPawn())return;if(FVector::Dist2D(GetPawn()->GetActorLocation(),ApproachPoint)>DestinationArrivalRadius){HandleRouteFailure(TEXT("Move finished before reaching road point"));return;}
 ++CompletedWalks;RecentGoals.Add(ApproachPoint);if(RecentGoals.Num()>6)RecentGoals.RemoveAt(0);Request=FAIRequestID::InvalidRequest;NextDecision=GetWorld()->GetTimeSeconds()+0.05;
 UE_LOG(LogForestRoadAI,Log,TEXT("%s: ROAM_REACHED %d, choose next road"),*GetNameSafe(GetPawn()),CompletedWalks);
}
void AForestRoadAIController::OnMoveCompleted(FAIRequestID ID,const FPathFollowingResult& Result)
{
 Super::OnMoveCompleted(ID,Result);if(bSuppressCompletion || bInteracting || !Request.IsValid() || ID!=Request)return;
 if(bApproachingPlayer){Request=FAIRequestID::InvalidRequest;NextDecision=GetWorld()->GetTimeSeconds()+(Result.IsSuccess()?0.1:0.6);StatusNote=Result.IsSuccess()?TEXT("Reached player"):TEXT("Finding another path to player");return;}
 if(Result.IsSuccess())CompleteWalk();else HandleRouteFailure(TEXT("Path following failed"));
}
bool AForestRoadAIController::BeginPlayerApproach(AActor* PlayerActor)
{
 if(!PlayerActor || !GetPawn() || bInteracting)return false;
 bSuppressCompletion=true;StopMovement();bSuppressCompletion=false;Request=FAIRequestID::InvalidRequest;
 ApproachTarget=PlayerActor;bApproachingPlayer=true;RecoveryUntil=0;NextDecision=0;StatusNote=TEXT("Looking for player");return true;
}
void AForestRoadAIController::CancelPlayerApproach()
{
 if(!bApproachingPlayer)return;
 bSuppressCompletion=true;StopMovement();bSuppressCompletion=false;Request=FAIRequestID::InvalidRequest;
 bApproachingPlayer=false;ApproachTarget.Reset();NextDecision=GetWorld()?GetWorld()->GetTimeSeconds()+0.3:0;StatusNote=TEXT("Resume road roaming");
}
void AForestRoadAIController::UpdatePlayerApproach(const double Now)
{
 auto* Target=ApproachTarget.Get();if(!Target || !GetPawn()){CancelPlayerApproach();return;}
 const FVector Here=GetPawn()->GetActorLocation();const float Distance=FVector::Dist2D(Here,Target->GetActorLocation());
 if(Distance<=140.f)
 {
  if(Request.IsValid()){bSuppressCompletion=true;StopMovement();bSuppressCompletion=false;Request=FAIRequestID::InvalidRequest;}
  const FRotator Facing=(Target->GetActorLocation()-GetPawn()->GetActorLocation()).GetSafeNormal2D().Rotation();GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(),Facing,GetWorld()->GetDeltaSeconds(),5));StatusNote=TEXT("Waiting to greet player");return;
 }
 if(Request.IsValid())
 {
  if(FVector::Dist2D(ProgressLocation,Here)>25){ProgressLocation=Here;ProgressTime=Now;}
  else if(Now-ProgressTime>StuckTimeout){bSuppressCompletion=true;StopMovement();bSuppressCompletion=false;Request=FAIRequestID::InvalidRequest;NextDecision=Now+0.5;StatusNote=TEXT("Retrying path to player");}
  return;
 }
 if(Now<NextDecision)return;
 FAIMoveRequest Move;Move.SetGoalActor(Target);Move.SetAcceptanceRadius(120);Move.SetReachTestIncludesAgentRadius(false);Move.SetReachTestIncludesGoalRadius(false);Move.SetAllowPartialPath(false);Move.SetNavigationFilter(DefaultNavigationFilterClass);Move.SetUsePathfinding(true);
 const auto Result=MoveTo(Move);Request=Result.MoveId;NextDecision=Now+0.5;ProgressLocation=Here;ProgressTime=Now;
 if(Result.Code==EPathFollowingRequestResult::Failed){Request=FAIRequestID::InvalidRequest;NextDecision=Now+0.8;StatusNote=TEXT("Finding a path to player");}else StatusNote=TEXT("Looking for player");
}
bool AForestRoadAIController::BeginInteraction(AActor* Interactor)
{
 if(!Interactor || !GetPawn() || bInteracting)return false;if(const auto* C=GetPawn()->FindComponentByClass<UForestAutoCrouchComponent>();C && C->IsInPassage())return false;
 CancelPlayerApproach();
 bInteracting=true;InteractionPartner=Interactor;bSuppressCompletion=true;StopMovement();bSuppressCompletion=false;Request=FAIRequestID::InvalidRequest;RecoveryUntil=0;StatusNote=TEXT("Talking to player");OnInteractionChanged.Broadcast(Interactor,true);
 UE_LOG(LogForestRoadAI,Log,TEXT("%s: INTERACTION_BEGIN"),*GetNameSafe(GetPawn()));return true;
}
void AForestRoadAIController::EndInteraction()
{
 if(!bInteracting)return;auto* Partner=InteractionPartner.Get();bInteracting=false;InteractionPartner.Reset();NextDecision=GetWorld()->GetTimeSeconds()+0.3;StatusNote=TEXT("Resume road roaming");OnInteractionChanged.Broadcast(Partner,false);
 UE_LOG(LogForestRoadAI,Log,TEXT("%s: INTERACTION_END"),*GetNameSafe(GetPawn()));
}
void AForestRoadAIController::RestartPatrol()
{EndInteraction();bSuppressCompletion=true;StopMovement();bSuppressCompletion=false;Request=FAIRequestID::InvalidRequest;RecoveryUntil=0;RebuildRoadNetwork();NextDecision=GetWorld()->GetTimeSeconds()+0.3;}
void AForestRoadAIController::Tick(float DT)
{
 Super::Tick(DT);DisableLegacyWanderTimer();if(!GetPawn())return;const double Now=GetWorld()->GetTimeSeconds();
 if(bInteracting){if(!InteractionPartner.IsValid()){EndInteraction();return;}const FRotator Facing=(InteractionPartner->GetActorLocation()-GetPawn()->GetActorLocation()).GetSafeNormal2D().Rotation();GetPawn()->SetActorRotation(FMath::RInterpTo(GetPawn()->GetActorRotation(),Facing,DT,5));return;}
 if(bApproachingPlayer){UpdatePlayerApproach(Now);return;}
 if(Now>=NextHistorySample){const FVector P=GetPawn()->GetActorLocation();if(MovementHistory.IsEmpty() || FVector::Dist2D(P,MovementHistory.Last())>50){MovementHistory.Add(P);if(MovementHistory.Num()>12)MovementHistory.RemoveAt(0);}NextHistorySample=Now+0.5;}
 if(auto* C=GetPawn()->FindComponentByClass<UForestAutoCrouchComponent>();C && C->IsInPassage()){ProgressTime=Now;ProgressLocation=GetPawn()->GetActorLocation();return;}
 if(Now<RecoveryUntil){StatusNote=TEXT("Backing away from obstruction");GetPawn()->AddMovementInput((RecoveryPoint-GetPawn()->GetActorLocation()).GetSafeNormal2D());if(FVector::Dist2D(RecoveryPoint,GetPawn()->GetActorLocation())<60)RecoveryUntil=0;return;}
 if(Request.IsValid())
 {if(FVector::Dist2D(ProgressLocation,GetPawn()->GetActorLocation())>25){ProgressLocation=GetPawn()->GetActorLocation();ProgressTime=Now;}else if(Now-ProgressTime>StuckTimeout){bSuppressCompletion=true;StopMovement();bSuppressCompletion=false;HandleRouteFailure(TEXT("No movement progress"));}}
 else if(Now>=NextDecision)StartRoaming();
}
FString AForestRoadAIController::GetRoadDebugStatus() const
{const auto* C=GetPawn()?GetPawn()->FindComponentByClass<UForestAutoCrouchComponent>():nullptr;return FString::Printf(TEXT("%s | completed walks %d | avoided routes %d | %s\n%s"),bInteracting?TEXT("Talking"):Request.IsValid()?TEXT("Moving"):TEXT("Choosing road"),CompletedWalks,RouteFailures,C?*C->GetCrouchStatus():TEXT("Standing"),*StatusNote);}
void AForestRoadAIController::DrawRoadDebug() const
{const auto Path=GetPathFollowingComponent()->GetPath();if(Path.IsValid()){const auto& Points=Path->GetPathPoints();for(int32 I=1;I<Points.Num();++I)DrawDebugLine(GetWorld(),Points[I-1].Location+FVector(0,0,20),Points[I].Location+FVector(0,0,20),FColor::Green,false,0.16f,0,3);}if(Request.IsValid())DrawDebugSphere(GetWorld(),ApproachPoint+FVector(0,0,30),60,12,FColor::Cyan,false,0.16f);}

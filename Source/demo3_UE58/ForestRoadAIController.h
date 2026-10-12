#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "ForestRoadAIController.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestNPCInteractionChanged,AActor*,Interactor,bool,bActive);
UCLASS(Blueprintable)
class DEMO3_UE58_API AForestRoadAIController : public AAIController
{
 GENERATED_BODY()
public:
 AForestRoadAIController(const FObjectInitializer& Init=FObjectInitializer::Get());
 virtual void Tick(float DT) override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Road Movement") FName RouteActorTag=TEXT("NPC_RoadRouteDraft");
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Road Movement") float RoadHalfWidth=180;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Road Movement") float DestinationArrivalRadius=100;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Road Movement") float MinRoamDistance=500;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Road Movement") float MaxRoamDistance=2500;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Road Movement") float StuckTimeout=5;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Road Movement") float AvoidDuration=45;
 UPROPERTY(BlueprintAssignable,Category="NPC Interaction") FForestNPCInteractionChanged OnInteractionChanged;
 UFUNCTION(BlueprintCallable,Category="Road Movement") void RebuildRoadNetwork();
 UFUNCTION(BlueprintCallable,Category="Road Movement") void RestartPatrol();
 UFUNCTION(BlueprintPure,Category="Road Movement") FString GetRoadDebugStatus() const;
 UFUNCTION(BlueprintCallable,Category="NPC Interaction") bool BeginInteraction(AActor* Interactor);
 UFUNCTION(BlueprintCallable,Category="NPC Interaction") void EndInteraction();
 UFUNCTION(BlueprintPure,Category="NPC Interaction") bool IsInteracting() const {return bInteracting;}
 UFUNCTION(BlueprintCallable,Category="NPC Interaction") bool BeginPlayerApproach(AActor* PlayerActor);
 UFUNCTION(BlueprintCallable,Category="NPC Interaction") void CancelPlayerApproach();
 UFUNCTION(BlueprintPure,Category="NPC Interaction") bool IsApproachingPlayer() const{return bApproachingPlayer;}
 void DrawRoadDebug() const;
protected:
 virtual void OnPossess(APawn* InPawn) override;
 virtual void OnUnPossess() override;
 virtual void OnMoveCompleted(FAIRequestID ID,const FPathFollowingResult& Result) override;
private:
 struct FCooldownPoint {FVector Location;double Until;};
 TArray<FVector> RoadPoints,RecentGoals,MovementHistory;
 TArray<FCooldownPoint> AvoidedPoints;
 TWeakObjectPtr<AActor> InteractionPartner;
 TWeakObjectPtr<AActor> ApproachTarget;
 FAIRequestID Request=FAIRequestID::InvalidRequest;
 FVector ApproachPoint=FVector::ZeroVector,ProgressLocation=FVector::ZeroVector,RecoveryPoint=FVector::ZeroVector;
 double ProgressTime=0,NextDecision=0,RecoveryUntil=0,NextHistorySample=0;
 int32 CompletedWalks=0,RouteFailures=0;
 bool bSuppressCompletion=false,bInteracting=false,bApproachingPlayer=false;
 FString StatusNote;
 void StartRoaming();
 void HandleRouteFailure(const FString& Reason);
 void CompleteWalk();
 void DisableLegacyWanderTimer();
 bool ResolvePoint(const FVector& Desired,FVector& OutPoint,float& OutLength) const;
 bool IsRecentlyUsed(const FVector& Point) const;
 void UpdatePlayerApproach(double Now);
};

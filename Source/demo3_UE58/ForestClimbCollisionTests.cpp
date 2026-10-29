#if WITH_DEV_AUTOMATION_TESTS
#include "ForestIsometricPlayer.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestClimbCollisionTest,"Forest.Climbing.ComponentCollision",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestClimbCollisionTest::RunTest(const FString&)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,FName(TEXT("ClimbCollisionTest")));
 if(!TestNotNull(TEXT("Isolated physics world"),World))return false;
 auto Box=[&](const FVector& Location,const FVector& Extent)
 {
  auto* Actor=World->SpawnActor<AActor>();
  auto* Component=NewObject<UBoxComponent>(Actor);Actor->SetRootComponent(Component);
  Component->SetBoxExtent(Extent);Component->SetMobility(EComponentMobility::Static);
  Component->SetCollisionProfileName(TEXT("BlockAll"));Component->ComponentTags.Add(TEXT("ForestClimbable"));
  Component->RegisterComponent();Actor->SetActorLocation(Location);return Component;
 };
 auto* Wall=Box(FVector(100,0,0),FVector(20,300,300));
 auto* OtherWall=Box(FVector(-300,0,0),FVector(20,300,300));
 auto* C=World->SpawnActor<ACharacter>();
 C->GetCapsuleComponent()->SetCapsuleSize(42,96);C->SetActorLocation(FVector::ZeroVector);
 auto* Actions=NewObject<UForestIsometricPlayer>(C);C->AddInstanceComponent(Actions);Actions->RegisterComponent();
 Actions->Character=C;Actions->ClimbNormal=FVector(-1,0,0);
 auto* Capsule=C->GetCapsuleComponent();const auto CollisionBefore=Capsule->GetCollisionEnabled();
 FHitResult Support;
 TestTrue(TEXT("Wall can be detected before ignoring movement"),Actions->FindClimbSurface(C->GetActorLocation(),FVector::ForwardVector,Support));
 TestTrue(TEXT("Install only the active surface ignore"),Actions->SetClimbCollisionSurface(Wall));
 TestTrue(TEXT("Independent wall detection survives movement ignore"),Actions->FindClimbSurface(C->GetActorLocation(),FVector::ForwardVector,Support));
 TestEqual(TEXT("Detection still identifies the ignored component"),Support.GetComponent(),static_cast<UPrimitiveComponent*>(Wall));
 FHitResult Hit;Actions->MoveClimbing(FVector(60,0,0),FQuat::Identity,Hit);
 TestTrue(TEXT("Outer capsule can overlap the current surface"),C->GetActorLocation().X>55.f);
 TestFalse(TEXT("Full collision check sees that overlap"),Actions->IsClimbCapsuleClear(C->GetActorLocation()));
 TestTrue(TEXT("Component transfer finds clearance before restoring old surface"),Actions->SetClimbCollisionSurface(OtherWall));
 TestFalse(TEXT("Old surface restored after transfer"),Capsule->GetMoveIgnoreComponents().Contains(Wall));
 TestTrue(TEXT("Only new surface remains ignored"),Capsule->GetMoveIgnoreComponents().Contains(OtherWall));
 TestTrue(TEXT("Transfer did not leave capsule embedded in old surface"),Actions->IsClimbCapsuleClear(C->GetActorLocation(),OtherWall));
 Actions->RestoreClimbCollision();
 C->SetActorLocation(FVector::ZeroVector);Actions->SetClimbCollisionSurface(Wall);
 Actions->MoveClimbing(FVector(200,0,0),FQuat::Identity,Hit);
 TestTrue(TEXT("Body core prevents crossing through the current wall"),Hit.bBlockingHit && C->GetActorLocation().X<70.f);
 Actions->bClimbing=true;C->GetCharacterMovement()->SetMovementMode(MOVE_Flying);Actions->StopClimbing();
 TestFalse(TEXT("Safe release ends climbing"),Actions->bClimbing);
 TestFalse(TEXT("Release restores original ignore state"),Capsule->GetMoveIgnoreComponents().Contains(Wall));
 TestTrue(TEXT("Release restores collision only at a clear position"),Actions->IsClimbCapsuleClear(C->GetActorLocation()));
 TestEqual(TEXT("Capsule collision mode never changed"),Capsule->GetCollisionEnabled(),CollisionBefore);
 auto* Obstacle=Box(FVector(0,120,0),FVector(200,20,300));
 C->SetActorLocation(FVector::ZeroVector);Actions->SetClimbCollisionSurface(Wall);
 Actions->MoveClimbing(FVector(0,200,0),FQuat::Identity,Hit);
 TestTrue(TEXT("Full capsule still blocks unrelated obstacles"),Hit.bBlockingHit && C->GetActorLocation().Y<65.f);
 TestFalse(TEXT("Obstacle never added to ignore list"),Capsule->GetMoveIgnoreComponents().Contains(Obstacle));
 Actions->RestoreClimbCollision();
 Capsule->IgnoreComponentWhenMoving(Wall,true);Actions->SetClimbCollisionSurface(Wall);Actions->RestoreClimbCollision();
 TestTrue(TEXT("Preexisting ignore relationship is preserved"),Capsule->GetMoveIgnoreComponents().Contains(Wall));
 Capsule->IgnoreComponentWhenMoving(Wall,false);
 Actions->SetClimbCollisionSurface(Wall);Actions->bClimbing=true;Actions->StopClimbing(true);
 TestFalse(TEXT("Forced teardown clears our ignore relationship"),Capsule->GetMoveIgnoreComponents().Contains(Wall));
 C->SetActorLocation(FVector(60,0,0));Actions->SetClimbCollisionSurface(Wall);Actions->bClimbing=true;
 auto* ExitBlocker=Box(FVector(-10,0,0),FVector(10,300,300));
 Actions->StopClimbing();
 TestTrue(TEXT("Blocked release remains climbing until a safe exit exists"),Actions->bClimbing && Actions->bClimbExitPending);
 TestTrue(TEXT("Blocked release retains the one active ignore"),Capsule->GetMoveIgnoreComponents().Contains(Wall));
 ExitBlocker->DestroyComponent();Actions->StopClimbing();
 TestFalse(TEXT("Pending release completes when clearance becomes available"),Actions->bClimbing);
 TestFalse(TEXT("Completed pending release removes ignore"),Capsule->GetMoveIgnoreComponents().Contains(Wall));
 C->SetActorLocation(FVector::ZeroVector);Actions->SetClimbCollisionSurface(Wall);Actions->bClimbing=true;
 Wall->DestroyComponent();Actions->StopClimbing();
 TestFalse(TEXT("Destroyed support ends climbing"),Actions->bClimbing);
 TestFalse(TEXT("Destroyed support is removed from movement ignore list"),Capsule->GetMoveIgnoreComponents().Contains(Wall));
 World->DestroyWorld(false);return true;
}
#endif

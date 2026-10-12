#include "PotionMimicWorldSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace PotionMimic
{
	constexpr int32 DesiredWorkbenchBottleCount = 7;
	constexpr float DiscoveryIntervalSeconds = 0.75f;
	constexpr float WorkbenchSearchRadius = 135.0f;
	constexpr float WorkbenchHeightTolerance = 65.0f;
	constexpr float AutomaticStartDelaySeconds = 2.0f;
	constexpr float BaseOrbitSpeedRadians = 0.72f;
	constexpr float MaximumBookSpeedForBoost = 500.0f;

	bool IsPotionMesh(const UStaticMesh* Mesh)
	{
		if (!Mesh)
		{
			return false;
		}

		const FString Name = Mesh->GetName();
		return Name.StartsWith(TEXT("SM_Potions_MDL_Potion"))
			|| Name.StartsWith(TEXT("SM_EmptyPotions_MDL_EmptyPotion"));
	}

	bool IsWorkbenchCauldronMesh(const UStaticMesh* Mesh)
	{
		return Mesh && Mesh->GetName().Contains(TEXT("CounterObjects_lo_Cauldron_lo"));
	}

	bool LooksLikeMimicBook(const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}

		const FString Identity = Actor->GetName() + TEXT(" ") + Actor->GetClass()->GetName();
		return Identity.Contains(TEXT("Mimic-book"), ESearchCase::IgnoreCase)
			|| Identity.Contains(TEXT("MimicBook"), ESearchCase::IgnoreCase);
	}
}

void UPotionMimicWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	TimeSinceBeginPlay = 0.0f;
	TimeSinceLastDiscovery = PotionMimic::DiscoveryIntervalSeconds;
	FlightBlend = 0.0f;
	FlightClock = 0.0f;
	DiscoverWorkbenchPotions();
	DiscoverMimicBook();
}

void UPotionMimicWorldSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	TimeSinceBeginPlay += DeltaTime;
	TimeSinceLastDiscovery += DeltaTime;

	const bool bNeedsBottleDiscovery = BoundPotions.Num() != PotionMimic::DesiredWorkbenchBottleCount;
	const bool bNeedsBookDiscovery = !MimicBookActor.IsValid();
	if ((bNeedsBottleDiscovery || bNeedsBookDiscovery)
		&& TimeSinceLastDiscovery >= PotionMimic::DiscoveryIntervalSeconds)
	{
		TimeSinceLastDiscovery = 0.0f;
		if (bNeedsBottleDiscovery)
		{
			DiscoverWorkbenchPotions();
		}
		if (bNeedsBookDiscovery)
		{
			DiscoverMimicBook();
		}
	}

	UpdateBookDrivenState(DeltaTime);
	UpdatePotionTransforms(DeltaTime);
}

TStatId UPotionMimicWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPotionMimicWorldSubsystem, STATGROUP_Tickables);
}

void UPotionMimicWorldSubsystem::StartWorkbenchPotionFlight(AActor* InMimicBookActor)
{
	SetMimicBookActor(InMimicBookActor);
	bFlightRequested = true;
}

void UPotionMimicWorldSubsystem::StopWorkbenchPotionFlight(bool bSnapHome)
{
	bFlightRequested = false;
	if (bSnapHome)
	{
		FlightBlend = 0.0f;
		for (FBoundPotion& Potion : BoundPotions)
		{
			RestorePotion(Potion, true);
		}
	}
}

void UPotionMimicWorldSubsystem::SetMimicBookActor(AActor* InMimicBookActor)
{
	MimicBookActor = InMimicBookActor;
	bHasSeenMimicBook = InMimicBookActor != nullptr;
	if (InMimicBookActor)
	{
		LastMimicBookLocation = InMimicBookActor->GetActorLocation();
	}
}

void UPotionMimicWorldSubsystem::SetAutomaticWorkbenchFlight(bool bEnabled)
{
	bAutomaticWorkbenchFlight = bEnabled;
	if (!bEnabled && !MimicBookActor.IsValid())
	{
		bFlightRequested = false;
	}
}

int32 UPotionMimicWorldSubsystem::GetBoundWorkbenchPotionCount() const
{
	return BoundPotions.Num();
}

void UPotionMimicWorldSubsystem::DiscoverWorkbenchPotions()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<AStaticMeshActor*> Potions;
	TArray<AStaticMeshActor*> Cauldrons;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* Actor = *It;
		const UStaticMeshComponent* MeshComponent = Actor ? Actor->GetStaticMeshComponent() : nullptr;
		const UStaticMesh* Mesh = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
		if (PotionMimic::IsPotionMesh(Mesh))
		{
			Potions.Add(Actor);
		}
		else if (PotionMimic::IsWorkbenchCauldronMesh(Mesh))
		{
			Cauldrons.Add(Actor);
		}
	}

	AStaticMeshActor* BestCauldron = nullptr;
	TArray<AStaticMeshActor*> BestCluster;
	int32 BestCountDifference = MAX_int32;
	float BestAverageDistance = TNumericLimits<float>::Max();

	for (AStaticMeshActor* Cauldron : Cauldrons)
	{
		TArray<TPair<AStaticMeshActor*, float>> Nearby;
		const FVector CauldronLocation = Cauldron->GetActorLocation();
		for (AStaticMeshActor* Potion : Potions)
		{
			const FVector Offset = Potion->GetActorLocation() - CauldronLocation;
			const float HorizontalDistance = FVector2D(Offset.X, Offset.Y).Size();
			if (HorizontalDistance <= PotionMimic::WorkbenchSearchRadius
				&& FMath::Abs(Offset.Z) <= PotionMimic::WorkbenchHeightTolerance)
			{
				Nearby.Emplace(Potion, HorizontalDistance);
			}
		}

		Nearby.Sort([](const TPair<AStaticMeshActor*, float>& Left, const TPair<AStaticMeshActor*, float>& Right)
		{
			return Left.Value < Right.Value;
		});

		const int32 CountDifference = FMath::Abs(Nearby.Num() - PotionMimic::DesiredWorkbenchBottleCount);
		float AverageDistance = 0.0f;
		for (const TPair<AStaticMeshActor*, float>& Pair : Nearby)
		{
			AverageDistance += Pair.Value;
		}
		AverageDistance = Nearby.IsEmpty() ? TNumericLimits<float>::Max() : AverageDistance / Nearby.Num();

		if (Nearby.Num() >= PotionMimic::DesiredWorkbenchBottleCount
			&& (CountDifference < BestCountDifference
				|| (CountDifference == BestCountDifference && AverageDistance < BestAverageDistance)))
		{
			BestCauldron = Cauldron;
			BestCountDifference = CountDifference;
			BestAverageDistance = AverageDistance;
			BestCluster.Reset();
			for (int32 Index = 0; Index < PotionMimic::DesiredWorkbenchBottleCount; ++Index)
			{
				BestCluster.Add(Nearby[Index].Key);
			}
		}
	}

	if (!BestCauldron || BestCluster.Num() != PotionMimic::DesiredWorkbenchBottleCount)
	{
		return;
	}

	for (FBoundPotion& Existing : BoundPotions)
	{
		RestorePotion(Existing, true);
	}
	BoundPotions.Reset();
	WorkbenchCauldron = BestCauldron;

	for (int32 Index = 0; Index < BestCluster.Num(); ++Index)
	{
		AStaticMeshActor* PotionActor = BestCluster[Index];
		UStaticMeshComponent* MeshComponent = PotionActor->GetStaticMeshComponent();
		FBoundPotion& Bound = BoundPotions.AddDefaulted_GetRef();
		Bound.Actor = PotionActor;
		Bound.HomeTransform = PotionActor->GetActorTransform();
		Bound.HomeCollision = MeshComponent->GetCollisionEnabled();
		Bound.Phase = (2.0f * PI * Index) / PotionMimic::DesiredWorkbenchBottleCount;
		Bound.Radius = 62.0f + 8.0f * (Index % 3);
		Bound.Height = 78.0f + 7.0f * (Index % 2);
		MeshComponent->SetMobility(EComponentMobility::Movable);
	}
}

void UPotionMimicWorldSubsystem::DiscoverMimicBook()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (PotionMimic::LooksLikeMimicBook(*It))
		{
			SetMimicBookActor(*It);
			return;
		}
	}
}

void UPotionMimicWorldSubsystem::UpdateBookDrivenState(float DeltaTime)
{
	if (AActor* Book = MimicBookActor.Get())
	{
		const FVector CurrentLocation = Book->GetActorLocation();
		const float RawSpeed = DeltaTime > SMALL_NUMBER
			? FVector::Distance(CurrentLocation, LastMimicBookLocation) / DeltaTime
			: 0.0f;
		MimicBookSpeed = FMath::FInterpTo(MimicBookSpeed, RawSpeed, DeltaTime, 4.0f);
		LastMimicBookLocation = CurrentLocation;
		bFlightRequested = !IsMimicBookFolded(Book);
		bHasSeenMimicBook = true;
		return;
	}

	MimicBookSpeed = FMath::FInterpTo(MimicBookSpeed, 0.0f, DeltaTime, 3.0f);
	if (bAutomaticWorkbenchFlight && !bHasSeenMimicBook
		&& TimeSinceBeginPlay >= PotionMimic::AutomaticStartDelaySeconds)
	{
		bFlightRequested = true;
	}
	else if (bHasSeenMimicBook)
	{
		bFlightRequested = false;
	}
}

void UPotionMimicWorldSubsystem::UpdatePotionTransforms(float DeltaTime)
{
	if (BoundPotions.IsEmpty())
	{
		return;
	}

	const float TargetBlend = bFlightRequested ? 1.0f : 0.0f;
	FlightBlend = FMath::FInterpConstantTo(FlightBlend, TargetBlend, DeltaTime, 0.55f);
	const float EasedBlend = FMath::InterpEaseInOut(0.0f, 1.0f, FlightBlend, 2.2f);
	const float SpeedBoost = 1.0f + 0.85f * FMath::Clamp(
		MimicBookSpeed / PotionMimic::MaximumBookSpeedForBoost,
		0.0f,
		1.0f);
	FlightClock += DeltaTime * SpeedBoost;

	const FVector Centre = GetFlightCentre();
	for (int32 Index = BoundPotions.Num() - 1; Index >= 0; --Index)
	{
		FBoundPotion& Potion = BoundPotions[Index];
		AStaticMeshActor* Actor = Potion.Actor.Get();
		if (!Actor)
		{
			BoundPotions.RemoveAtSwap(Index);
			continue;
		}

		UStaticMeshComponent* MeshComponent = Actor->GetStaticMeshComponent();
		if (FlightBlend > KINDA_SMALL_NUMBER)
		{
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		const float Angle = FlightClock * PotionMimic::BaseOrbitSpeedRadians + Potion.Phase;
		const float BreathingRadius = Potion.Radius + 9.0f * FMath::Sin(Angle * 1.7f + Potion.Phase);
		const FVector OrbitOffset(
			FMath::Cos(Angle) * BreathingRadius,
			FMath::Sin(Angle) * BreathingRadius,
			Potion.Height + 18.0f * FMath::Sin(Angle * 1.9f + Potion.Phase * 0.5f));
		const FVector FlyingLocation = Centre + OrbitOffset;
		const FVector TargetLocation = FMath::Lerp(Potion.HomeTransform.GetLocation(), FlyingLocation, EasedBlend);

		const FRotator HomeRotation = Potion.HomeTransform.Rotator();
		const FRotator FlyingRotation(
			12.0f * FMath::Sin(Angle * 1.4f),
			FMath::RadiansToDegrees(Angle) + 90.0f,
			10.0f * FMath::Cos(Angle * 1.1f + Potion.Phase));
		const FQuat TargetRotation = FQuat::Slerp(HomeRotation.Quaternion(), FlyingRotation.Quaternion(), EasedBlend);
		Actor->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);

		if (FlightBlend <= KINDA_SMALL_NUMBER && !bFlightRequested)
		{
			RestorePotion(Potion, true);
		}
	}
}

void UPotionMimicWorldSubsystem::RestorePotion(FBoundPotion& Potion, bool bSnapHome)
{
	AStaticMeshActor* Actor = Potion.Actor.Get();
	if (!Actor)
	{
		return;
	}

	if (bSnapHome)
	{
		Actor->SetActorTransform(Potion.HomeTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	Actor->GetStaticMeshComponent()->SetCollisionEnabled(Potion.HomeCollision);
}

bool UPotionMimicWorldSubsystem::IsMimicBookFolded(const AActor* Actor) const
{
	if (!Actor)
	{
		return true;
	}

	const FBoolProperty* FoldedProperty = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("isFolded"));
	if (!FoldedProperty)
	{
		FoldedProperty = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("IsFolded"));
	}
	return FoldedProperty ? FoldedProperty->GetPropertyValue_InContainer(Actor) : false;
}

FVector UPotionMimicWorldSubsystem::GetFlightCentre() const
{
	if (const AActor* Book = MimicBookActor.Get())
	{
		return Book->GetActorLocation() + FVector(0.0f, 0.0f, 15.0f);
	}
	if (const AActor* Cauldron = WorkbenchCauldron.Get())
	{
		return Cauldron->GetActorLocation();
	}

	FVector Average = FVector::ZeroVector;
	int32 ValidCount = 0;
	for (const FBoundPotion& Potion : BoundPotions)
	{
		Average += Potion.HomeTransform.GetLocation();
		++ValidCount;
	}
	return ValidCount > 0 ? Average / ValidCount : FVector::ZeroVector;
}

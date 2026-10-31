#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "ForestWorldZones.generated.h"
class AForestElementResource;

/** State data is authoritative. AI narrative never writes these properties. */
USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestZoneState
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName StateId=TEXT("normal");
 UPROPERTY(EditAnywhere,BlueprintReadWrite) float ResourceAvailabilityScale=1.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxAvailableResources=-1;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bHarvestEnabled=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bHideUnavailableResources=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bHideTaggedActors=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bDisableTaggedActorCollision=false;
 // Reserved preset keys for existing/future NPC and environment consumers.
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName NpcStateId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName MapStateId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FString SourceReceiptId;
};
USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestZoneControlRow
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ZoneId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 TargetDay=1;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bEnabled=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FForestZoneState State;
};
USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestZonePolygon
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadWrite) FName ZoneId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite) TArray<FVector2D> Points;
};
USTRUCT()
struct FForestZoneLayout
{
 GENERATED_BODY()
 UPROPERTY() int32 SchemaVersion=1;
 UPROPERTY() FVector HomeCenter=FVector(44820,13210,-8950);
 UPROPERTY() float HomeProtectionRadius=900.f;
 UPROPERTY() float EntryHalfAngleDegrees=60.f;
 UPROPERTY() int32 ProtectedResourcesPerProductionRegion=1;
 UPROPERTY() TArray<FForestZonePolygon> PolygonOverrides;
 UPROPERTY() TArray<FForestZoneControlRow> Controls;
};
USTRUCT(BlueprintType)
struct DEMO3_UE58_API FForestWorldZone
{
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FName ZoneId;
 UPROPERTY(BlueprintReadOnly) FString Label;
 UPROPERTY(BlueprintReadOnly) FString ProductionAnchorId;
 UPROPERTY(BlueprintReadOnly) FVector Center=FVector::ZeroVector;
 UPROPERTY(BlueprintReadOnly) FVector2D EntryDirection=FVector2D(1,0);
 UPROPERTY(BlueprintReadOnly) bool bProtected=false;
};
UCLASS()
class UForestWorldZoneSave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() TArray<FForestZoneControlRow> Controls;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FForestZoneStateChanged,FName,ZoneId,const FForestZoneState&,State);

/** Invisible spatial membership + explicit, day-scoped controls. Neutral by default. */
UCLASS()
class DEMO3_UE58_API UForestWorldZones : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Tick(float DeltaTime) override;
 virtual void Deinitialize() override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickableInEditor() const override{return false;}
 UFUNCTION(BlueprintPure,Category="Forest|Zones") TArray<FForestWorldZone> GetZones() const{return Zones;}
 UFUNCTION(BlueprintPure,Category="Forest|Zones") FName GetZoneAtLocation(FVector Position) const;
 UFUNCTION(BlueprintPure,Category="Forest|Zones") FName GetActorZone(AActor* Actor) const;
 UFUNCTION(BlueprintPure,Category="Forest|Zones") FForestZoneState GetZoneState(FName ZoneId,int32 Day=0) const;
 UFUNCTION(BlueprintCallable,Category="Forest|Zones") bool SetZoneState(FName ZoneId,const FForestZoneState& State,int32 TargetDay=0);
 UFUNCTION(BlueprintCallable,Category="Forest|Zones") bool ResetZone(FName ZoneId,int32 TargetDay=0);
 UFUNCTION(BlueprintCallable,Category="Forest|Zones") bool ReloadLayout();
 UFUNCTION(BlueprintCallable,Category="Forest|Zones") bool SetActorZoneControlled(AActor* Actor,bool bControlled);
 UPROPERTY(BlueprintAssignable,Category="Forest|Zones") FForestZoneStateChanged OnZoneStateChanged;
 void BeginResourceDay(int32 Day);
 void RegisterResource(AForestElementResource* Resource,const FString& AnchorId,int32 ProductionOrdinal);
private:
 UPROPERTY() TObjectPtr<UForestWorldZoneSave> Save;
 UPROPERTY() TArray<FForestWorldZone> Zones;
 FForestZoneLayout Layout;
 TArray<TWeakObjectPtr<AForestElementResource>> Resources;
 TMap<FName,int32> ZoneOrdinals;
 struct FActorOriginal {TWeakObjectPtr<AActor> Actor;bool Hidden=false,Collision=true;};
 TMap<TWeakObjectPtr<AActor>,FActorOriginal> ActorOriginals;
 int32 ResourceDay=0,LastDay=0;
 float ScanTime=0;
 bool bLoaded=false;
 bool ActiveWorld() const;
 int32 CurrentDay() const;
 bool DiscoverZones();
 FName Locate(FVector Position,const FString& AnchorId=FString()) const;
 void ApplyResource(AForestElementResource* Resource);
 void ReassignResources();
 void ApplyControls();
 void RestoreActors();
 bool HasActorControls() const;
};

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "ForestElementCodex.generated.h"

class UForestElementCodex;class UInputComponent;class USkeletalMesh;class UAnimSequence;
class SBox;class SHorizontalBox;class SVerticalBox;
struct FForestCodexEntry
{
 FString Element,Title,Description,Reference;
 int32 Strength=1;
};
UCLASS()
class DEMO3_UE58_API UForestElementCodexWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<UForestElementCodex> Owner;
 void Refresh();
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual void NativeTick(const FGeometry&,float) override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
private:
 TSharedPtr<SBox> Frame;
 TSharedPtr<SVerticalBox> Body;
 bool bSinglePage=false;
};
/** Read-only, table-backed element reference. Contains no commission targets. */
UCLASS()
class DEMO3_UE58_API UForestElementCodex : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 UForestElementCodex();
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickableInEditor() const override{return false;}
 virtual void Deinitialize() override;
 UFUNCTION(BlueprintCallable,Category="Magic Book") void Toggle();
 UFUNCTION(BlueprintCallable,Category="Magic Book") void Open();
 UFUNCTION(BlueprintCallable,Category="Magic Book") void Close();
 UFUNCTION(BlueprintPure,Category="Magic Book") bool IsOpen() const;
 void Select(int32 Element,int32 Strength);
 void TurnPage(int32 Direction);
 const FForestCodexEntry* CurrentEntry() const;
 FString ReferenceFor(int32 Element,int32 Strength) const;
 TArray<FForestCodexEntry> Entries;
 FString Introduction,Notes,LoadError;
 int32 SelectedElement=0,SelectedStrength=1;
private:
 UPROPERTY() TObjectPtr<UInputComponent> Input;
 UPROPERTY() TObjectPtr<UForestElementCodexWidget> Widget;
 UPROPERTY() TObjectPtr<USkeletalMesh> BookMesh;
 UPROPERTY() TObjectPtr<UAnimSequence> OpenAnimation;
 UPROPERTY() TObjectPtr<UAnimSequence> FoldAnimation;
 TWeakObjectPtr<APlayerController> Player;
 TWeakObjectPtr<AActor> BookVisual;
 bool bReady=false,bOldCursor=false;
 float OpeningTime=0;
 void Setup();
 void LoadEntries();
 void PlayBook(bool bOpening);
 bool OtherMenuOpen() const;
};

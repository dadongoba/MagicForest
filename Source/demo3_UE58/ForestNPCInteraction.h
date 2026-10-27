#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "Subsystems/WorldSubsystem.h"
#include "ForestTravelNarrativeSubsystem.h"
#include "ForestNPCInteraction.generated.h"
class SButton;class SEditableTextBox;class SScrollBox;class STextBlock;class SWidgetSwitcher;class SBox;class SWrapBox;
class AAIController;class ACharacter;class APlayerController;class AForestRoadAIController;class AForestNPCInteraction;class UForestNPCDialogueSubsystem;
UCLASS()
class DEMO3_UE58_API UForestNPCInteractionWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<AForestNPCInteraction> Manager;
 void ShowMessage(const FString& Name,const FString& Body,const FString& Hint);
 void ShowConversation(const FString& Name,const FString& Body,const FString& Status,bool Waiting);
 void ShowFinderSelection();
 void ShowFinderConfirmation(const FString& CharacterName,const TArray<FString>& Items);
 void ShowSelectedFee(const FString& FeeItem);
 void ShowTravel(const FString& CharacterName,const FString& FeeItem,const FString& Story);
 bool IsFinderVisible() const{return bFinderVisible;}
 void FocusInput();
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual void NativeTick(const FGeometry&,float) override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
 TSharedPtr<SWidget> Root;
 TSharedPtr<STextBlock> Title;
 TSharedPtr<STextBlock> Message;
 TSharedPtr<STextBlock> Prompt;
 TSharedPtr<SEditableTextBox> Input;
 TSharedPtr<SButton> Send;
 TSharedPtr<SScrollBox> Scroll;
 enum class EPanel:uint8 {Notice,Conversation,Selecting,Confirming,Travelling};
 EPanel Panel=EPanel::Notice;
 void SetPanel(EPanel Mode);
 TSharedPtr<SBox> AdaptivePanel,PanelFrame;
 float LayoutWidth=1100.f;
 TSharedPtr<SWidgetSwitcher> Switcher;
 TSharedPtr<STextBlock> Subtitle;
 TSharedPtr<SWrapBox> FinderGrid;
 TSharedPtr<STextBlock> FinderTargetName,FinderItemName,FinderItemAttribute,FinderItemDescription,FinderFee;
 TSharedPtr<STextBlock> TravelTarget,TravelStory,TravelFee;
 TSharedPtr<SButton> FinderConfirm;
 FName SelectedFinderPersona;
 FString SelectedFee;
 FString LastTravelTarget,LastTravelFee,LastTravelStory;
 bool bFinderVisible=false;
 UFUNCTION() void SendClicked();
 UFUNCTION() void CloseClicked();
 UFUNCTION() void FindNovelistClicked();
 UFUNCTION() void FindBlacksmithClicked();
 UFUNCTION() void FindMessengerClicked();
 UFUNCTION() void FinderConfirmClicked();
 UFUNCTION() void FinderCancelClicked();
};
UCLASS(Blueprintable)
class DEMO3_UE58_API AForestNPCInteraction : public AActor
{
 GENERATED_BODY()
public:
 AForestNPCInteraction();
 virtual void Tick(float DT) override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="NPC Interaction") float InteractionRange=250;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="NPC Dialogue") bool bOfflineTestMode=false;
 UFUNCTION(BlueprintCallable,Category="NPC Interaction") bool TryInteractWith(ACharacter* Character);
 UFUNCTION(BlueprintCallable,Category="NPC Interaction") void FinishInteraction();
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") bool SubmitDialogueText(const FString& Text);
 UFUNCTION(BlueprintPure,Category="NPC Interaction") bool IsDialogueOpen() const{return Active.IsValid();}
 bool IsFinderMenuOpen() const{return FinderStage==EFinderStage::Selecting || FinderStage==EFinderStage::Confirming;}
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") FString GetDialogueStatus() const;
 UFUNCTION(BlueprintCallable,Category="NPC Finder") void OpenNpcFinder();
 UFUNCTION(BlueprintCallable,Category="NPC Finder") void CancelNpcFinder();
 void SelectNpcForFinding(FName PersonaId);
 void ConfirmNpcFinding();
 void SelectFeeItem(const FString& Item);
protected:
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 TWeakObjectPtr<APlayerController> Player;
 TWeakObjectPtr<AAIController> Active;
 TWeakObjectPtr<ACharacter> Nearby;
 TWeakObjectPtr<ACharacter> TutorialMessenger;
 UPROPERTY(Transient) TObjectPtr<UForestNPCInteractionWidget> Widget;
 UPROPERTY(Transient) TObjectPtr<UForestNPCDialogueSubsystem> Dialogue;
 bool bLockedMovement=false,bPreviousMouse=false,bSuppressInteractUntilRelease=false,bInputBound=false,bTutorialSearchIssued=false;
 enum class EFinderStage:uint8 {Closed,Selecting,Confirming,Travelling};
 EFinderStage FinderStage=EFinderStage::Closed;
 FName PendingFinderPersona;
 FString PendingFeeItem,SpentFeeItem,FinderNotice;
 FString EmergentTravelItemName,TravelNarrativeText;
 TWeakObjectPtr<ACharacter> FinderTarget;
 bool bFinderPreviousMouse=false;
 double FinderNextMove=0,FinderNoticeUntil=0;
 int64 NextTravelNarrativeRequestId=1,ActiveTravelNarrativeRequestId=0;
 double TutorialSearchAfter=0;
 bool CanReach(ACharacter* Character) const;
 FString CharacterName(const ACharacter* Character) const;
 FName PersonaId(const ACharacter* Character) const;
 bool OpenInteraction(ACharacter* Character,bool bFirstDayTutorial);
 void UpdateFirstDayTutorial();
 void ToggleNpcFinder();
 void UpdateNpcFinder();
 void UpdateNpcFinderFacing(float DeltaTime);
 void CloseNpcFinderMenu();
 void StopNpcFinder(const FString& Notice,bool bRefundFee);
 ACharacter* FindCharacterByPersona(FName Persona) const;
 TArray<FString> GetFeeItems() const;
 bool ConsumeFeeItem(const FString& Item);
 void RefundFeeItem(const FString& Item);
 void RebuildBackpackList() const;
 bool HasCompletePathTo(ACharacter* Character) const;
 void RequestTravelNarrative(const FString& ConsumedItem,ACharacter* Target);
 UFUNCTION() void HandleTravelNarrative(const FForestTravelNarrativeResult& Result);
 UFUNCTION() void RefreshConversation();
 UFUNCTION() void InteractNearby();
};

/** Keeps player shortcuts available independently of an NPC controller spawning them. */
UCLASS()
class DEMO3_UE58_API UForestNPCInteractionWorldSubsystem : public UTickableWorldSubsystem
{
 GENERATED_BODY()
public:
 virtual void OnWorldBeginPlay(UWorld& InWorld) override;
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override;
 virtual bool IsTickableInEditor() const override{return false;}
private:
 float BootstrapDelay=0;
};

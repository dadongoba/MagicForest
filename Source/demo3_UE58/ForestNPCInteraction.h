#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "ForestNPCInteraction.generated.h"
class UTextBlock;class UEditableTextBox;class UButton;class UScrollBox;class UVerticalBox;
class ACharacter;class APlayerController;class AForestRoadAIController;class AForestNPCInteraction;class UForestNPCDialogueSubsystem;
UCLASS()
class DEMO3_UE58_API UForestNPCInteractionWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<AForestNPCInteraction> Manager;
 void ShowMessage(const FString& Name,const FString& Body,const FString& Hint);
 void ShowConversation(const FString& Name,const FString& Body,const FString& Status,bool Waiting);
 void ShowFinderSelection();
 void ShowFinderConfirmation(const FString& CharacterName,const FString& FeeItem);
 bool IsFinderVisible() const{return bFinderVisible;}
 void FocusInput();
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Message;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> Prompt;
 UPROPERTY(Transient) TObjectPtr<UEditableTextBox> Input;
 UPROPERTY(Transient) TObjectPtr<UButton> Send;
 UPROPERTY(Transient) TObjectPtr<UButton> Close;
 UPROPERTY(Transient) TObjectPtr<UScrollBox> Scroll;
 UPROPERTY(Transient) TObjectPtr<UVerticalBox> Conversation;
 UPROPERTY(Transient) TObjectPtr<UVerticalBox> FinderSelection;
 UPROPERTY(Transient) TObjectPtr<UVerticalBox> FinderConfirmation;
 UPROPERTY(Transient) TObjectPtr<UTextBlock> FinderFee;
 UPROPERTY(Transient) TObjectPtr<UButton> FinderConfirm;
 bool bFinderVisible=false;
 UFUNCTION() void SendClicked();
 UFUNCTION() void CloseClicked();
 UFUNCTION() void FindNovelistClicked();
 UFUNCTION() void FindBlacksmithClicked();
 UFUNCTION() void FindMessengerClicked();
 UFUNCTION() void FinderConfirmClicked();
 UFUNCTION() void FinderCancelClicked();
 UFUNCTION() void TextCommitted(const FText& Text,ETextCommit::Type Method);
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
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") FString GetDialogueStatus() const;
 UFUNCTION(BlueprintCallable,Category="NPC Finder") void OpenNpcFinder();
 UFUNCTION(BlueprintCallable,Category="NPC Finder") void CancelNpcFinder();
 void SelectNpcForFinding(FName PersonaId);
 void ConfirmNpcFinding();
protected:
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 TWeakObjectPtr<APlayerController> Player;
 TWeakObjectPtr<AForestRoadAIController> Active;
 TWeakObjectPtr<ACharacter> Nearby;
 TWeakObjectPtr<ACharacter> TutorialMessenger;
 UPROPERTY(Transient) TObjectPtr<UForestNPCInteractionWidget> Widget;
 UPROPERTY(Transient) TObjectPtr<UForestNPCDialogueSubsystem> Dialogue;
 bool bLockedMovement=false,bPreviousMouse=false,bSuppressInteractUntilRelease=false,bInputBound=false,bTutorialSearchIssued=false;
 enum class EFinderStage:uint8 {Closed,Selecting,Confirming,Travelling};
 EFinderStage FinderStage=EFinderStage::Closed;
 FName PendingFinderPersona;
 FString PendingFeeItem,SpentFeeItem,FinderNotice;
 TWeakObjectPtr<ACharacter> FinderTarget;
 bool bFinderPreviousMouse=false;
 double FinderNextMove=0,FinderNoticeUntil=0;
 double TutorialSearchAfter=0;
 bool CanReach(ACharacter* Character) const;
 FString CharacterName(const ACharacter* Character) const;
 FName PersonaId(const ACharacter* Character) const;
 bool OpenInteraction(ACharacter* Character,bool bFirstDayTutorial);
 void UpdateFirstDayTutorial();
 void ToggleNpcFinder();
 void UpdateNpcFinder();
 void CloseNpcFinderMenu();
 void StopNpcFinder(const FString& Notice,bool bRefundFee);
 ACharacter* FindCharacterByPersona(FName Persona) const;
 bool FindFeeItem(FString& OutItem) const;
 bool ConsumeFeeItem(const FString& Item);
 void RefundFeeItem(const FString& Item);
 void RebuildBackpackList() const;
 bool HasCompletePathTo(ACharacter* Character) const;
 UFUNCTION() void RefreshConversation();
 UFUNCTION() void InteractNearby();
};

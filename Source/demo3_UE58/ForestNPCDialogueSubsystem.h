#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "AlchemyGameplayTypes.h"
#include "ForestNPCDialogueSubsystem.generated.h"
class UForestNPCDialogueSubsystem;
USTRUCT()
struct FForestConversationRow
{
 GENERATED_BODY()
 UPROPERTY() FName NpcId;
 UPROPERTY() TArray<FDialogueMessage> History;
 UPROPERTY() int64 NextSession=1;
 UPROPERTY() int64 PendingSession=0;
 UPROPERTY() TArray<FDialogueMessage> PendingTranscript;
};
UCLASS()
class DEMO3_UE58_API UForestConversationSave : public USaveGame
{
 GENERATED_BODY()
public:
 UPROPERTY() TArray<FForestConversationRow> Rows;
 UPROPERTY() bool bFirstDayMessengerTutorialDelivered=false;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FForestConversationUpdated);
UCLASS()
class DEMO3_UE58_API UForestDialogueRequestContext : public UObject
{
 GENERATED_BODY()
public:
 TWeakObjectPtr<UForestNPCDialogueSubsystem> Owner;
 FName NpcId;
 int64 Session=0;
 int32 Generation=0;
 bool bSummary=false;
 UFUNCTION() void Completed(bool bSuccess,const FString& Text);
};
/** Per-NPC conversations; delegates keep network responses scoped to a session. */
UCLASS()
class DEMO3_UE58_API UForestNPCDialogueSubsystem : public UGameInstanceSubsystem
{
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 virtual void Deinitialize() override;
 UPROPERTY(BlueprintAssignable) FForestConversationUpdated OnConversationUpdated;
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") bool BeginConversation(FName NpcId);
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") bool BeginFirstDayMessengerTutorial();
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") bool IsFirstDayMessengerTutorialPending() const;
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") bool SendPlayerMessage(const FString& Text);
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") void EndConversation();
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") void SetOfflineTestMode(bool Enabled){bOfflineTest=Enabled;}
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") bool IsOfflineTestMode() const{return bOfflineTest;}
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") bool IsWaiting() const{return bWaiting;}
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") FString GetTranscript() const;
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") FString GetStatus() const{return Status;}
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") FName GetActiveNpcId() const{return ActiveNpc;}
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") FString GetCompactMemoryJson(FName NpcId) const;
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") bool ConfigurePersistence(const FString& ConversationSlot,const FString& MemorySlot);
 UFUNCTION(BlueprintCallable,Category="NPC Dialogue") void RetryPendingSummaries();
 UFUNCTION(BlueprintPure,Category="NPC Dialogue") int32 GetPendingSummaryCount() const;
 void HandleResponse(UForestDialogueRequestContext* Context,bool Success,const FString& Text);
private:
 UPROPERTY() TObjectPtr<UForestConversationSave> Save;
 UPROPERTY() TArray<TObjectPtr<UForestDialogueRequestContext>> Requests;
 FName ActiveNpc;
 FString Slot=TEXT("ForestNPCConversations"),Status;
 TArray<FDialogueMessage> SessionTranscript;
 int32 Generation=0;
 bool bWaiting=false,bOfflineTest=false,bPersistenceReady=false;
 FForestConversationRow& Row(FName Id);
 bool Persist();
 void RegisterForestPersonas();
 void RequestSummary(FForestConversationRow& Record);
 FString OfflineReply(const FString& Text) const;
 FString OfflinePatch(const FForestConversationRow& Record) const;
};

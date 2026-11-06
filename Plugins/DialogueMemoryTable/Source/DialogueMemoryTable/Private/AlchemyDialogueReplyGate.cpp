#include "AlchemyDialogueReplyGate.h"
#include "AlchemyDialogueKnowledge.h"
#include "AlchemyPersonaPolicy.h"

void UAlchemyDialogueReplyGate::Send()
{
 if(!Service.IsValid())return;
 FDialogueAICompletion Callback;Callback.BindDynamic(this,&UAlchemyDialogueReplyGate::Completed);
 Service->RequestChatCompletion(Messages,.7f,400,Callback);
}
void UAlchemyDialogueReplyGate::Completed(bool Success,const FString& Text)
{
 if(!Service.IsValid())return;
 FString Safe=Success?AlchemyPersonaPolicy::CleanReply(PersonaId,Text):Text;
 if(Success && !AlchemyDialogueKnowledge::IsSafeReplyForPlayer(Safe,PlayerText))
 {
  if(Repairs++==0)
  {
   Messages[0].Content+=TEXT("\n重新用已提供的用途和性质回答，禁止数字、确认或否认玩家猜的数字；不说明校验过程。\n");Send();return;
  }
  Safe=AlchemyPersonaPolicy::CleanReply(PersonaId,FString());
 }
 const auto Callback=Result;
 Service->ReleaseNpcReplyGate(this);
 Callback.ExecuteIfBound(Success,Safe);
}

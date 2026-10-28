#include "ForestNPCSetupLibrary.h"
#include "ForestCrouchNavigation.h"
#include "ForestHarvestMagic.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "NavLinkCustomComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TargetPoint.h"
#if WITH_EDITOR
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_LinkedInputPose.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_ModifyBone.h"
#include "K2Node_VariableGet.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#endif

FString UForestNPCSetupLibrary::ConfigureNPCCollision(UObject* Context)
{
#if WITH_EDITOR
 UWorld* W=Context?Context->GetWorld():nullptr;if(!W)return TEXT("No world");
 int32 Small=0,Decoration=0,Large=0,NPCs=0;
 for(TActorIterator<AActor> It(W);It;++It)
 {
  if(auto* C=Cast<ACharacter>(*It);C && C->GetClass()->GetName().StartsWith(TEXT("BP_NPC_")))
  {C->Modify();C->GetCapsuleComponent()->Modify();C->GetCapsuleComponent()->SetCollisionProfileName(TEXT("ForestNPCPawn"));C->MarkPackageDirty();++NPCs;}
  TArray<UStaticMeshComponent*> Meshes;It->GetComponents<UStaticMeshComponent>(Meshes);
  for(auto* Mesh:Meshes)
  {
   if(!Mesh->GetStaticMesh())continue;
   const FString Name=Mesh->GetStaticMesh()->GetName();
   if(Name.StartsWith(TEXT("SM_Env_Mushroom")))
   {
    Mesh->Modify();UForestHarvestMagic::ConfigureMushroomCollision(Mesh);It->MarkPackageDirty();++Decoration;continue;
   }
   if(Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision)continue;
   const FVector Size=Mesh->GetStaticMesh()->GetBoundingBox().GetSize()*Mesh->GetComponentScale().GetAbs();
   const bool ExistingDecoration=Mesh->GetCollisionProfileName()==TEXT("ForestDecoration");
   const bool Foliage=Name.StartsWith(TEXT("SM_Env_Ground_Grass_")) || Name.StartsWith(TEXT("SM_Env_Vines_"));
   const bool Prop=Name.StartsWith(TEXT("SM_Env_Branch_")) || Name.StartsWith(TEXT("SM_Env_Roots_")) ||
    Name.StartsWith(TEXT("SM_Env_Mushroom_")) || Name.StartsWith(TEXT("SM_Prop_Crystal_")) ||
    Name.StartsWith(TEXT("SM_Prop_Geode_")) || Name.StartsWith(TEXT("SM_Prop_Lantern_"));
   const bool SmallProp=Prop && Size.Z<=100.f && Size.GetMax()<=180.f;
   const bool KeepLarge=It->ActorHasTag(TEXT("NPC_LargeObstacle"));
   if(!KeepLarge && (ExistingDecoration || Foliage || SmallProp || It->ActorHasTag(TEXT("NPC_SmallDecoration"))))
   {
    Mesh->Modify();Mesh->SetCollisionProfileName(ExistingDecoration?TEXT("ForestDecoration"):TEXT("ForestSmallProp"));
    Mesh->SetCanEverAffectNavigation(false);It->MarkPackageDirty();
    if(ExistingDecoration)++Decoration;else ++Small;
   }
   else ++Large;
  }
 }
 return FString::Printf(TEXT("NPCs %d; existing decoration %d; additional small scenery %d; retained obstacles %d"),NPCs,Decoration,Small,Large);
#else
 return TEXT("Editor only");
#endif
}

bool UForestNPCSetupLibrary::ConfigureCrouchPose(UObject* Object)
{
#if WITH_EDITOR
 auto* BP=Cast<UAnimBlueprint>(Object);if(!BP)return false;
 UEdGraph* Graph=nullptr;for(UEdGraph* G:BP->FunctionGraphs)if(G->GetFName()==TEXT("AnimGraph"))Graph=G;
 if(!Graph)return false;
 BP->Modify();Graph->Modify();
 TArray<UEdGraphNode*> Old=Graph->Nodes;for(auto* Node:Old)FBlueprintEditorUtils::RemoveNode(BP,Node,true);
 auto Make=[Graph](UClass* Class,int32 X)->UEdGraphNode*
 {auto* N=NewObject<UEdGraphNode>(Graph,Class,NAME_None,RF_Transactional);Graph->AddNode(N,false,false);N->CreateNewGuid();N->PostPlacedNewNode();N->NodePosX=X;N->AllocateDefaultPins();return N;};
 auto* Input=Cast<UAnimGraphNode_LinkedInputPose>(Make(UAnimGraphNode_LinkedInputPose::StaticClass(),0));
 auto* Local=Make(UAnimGraphNode_LocalToComponentSpace::StaticClass(),220);
 auto* Getter=Cast<UK2Node_VariableGet>(NewObject<UK2Node_VariableGet>(Graph));
 Graph->AddNode(Getter,false,false);Getter->CreateNewGuid();Getter->VariableReference.SetSelfMember(TEXT("CrouchAlpha"));Getter->AllocateDefaultPins();Getter->NodePosY=300;
 auto Connect=[Graph](UEdGraphNode* A,UEdGraphNode* B,const FName InputName=NAME_None)
 {
  UEdGraphPin* Out=nullptr;UEdGraphPin* In=nullptr;
  for(auto* P:A->Pins)if(P->Direction==EGPD_Output && P->PinType.PinCategory==UEdGraphSchema_K2::PC_Struct){Out=P;break;}
  for(auto* P:B->Pins)if(P->Direction==EGPD_Input && P->PinType.PinCategory==UEdGraphSchema_K2::PC_Struct && (InputName.IsNone() || P->PinName==InputName)){In=P;break;}
  return Out && In && Graph->GetSchema()->TryCreateConnection(Out,In);
 };
 if(!Connect(Input,Local))return false;
 UEdGraphNode* Previous=Local;int32 X=440;
 const TArray<TPair<FName,float>> Bones={{TEXT("pelvis"),0},{TEXT("thigh_l"),35},{TEXT("thigh_r"),35},{TEXT("calf_l"),-70},{TEXT("calf_r"),-70},{TEXT("foot_l"),35},{TEXT("foot_r"),35}};
 for(const auto& Bone:Bones)
 {
  auto* Node=Cast<UAnimGraphNode_ModifyBone>(NewObject<UAnimGraphNode_ModifyBone>(Graph,NAME_None,RF_Transactional));
  Graph->AddNode(Node,false,false);Node->CreateNewGuid();Node->PostPlacedNewNode();Node->NodePosX=X;X+=220;
  Node->Node.BoneToModify.BoneName=Bone.Key;
  // The bent legs already shorten the stance. A 36 cm pelvis drop put
  // the toes below the capsule floor after the mesh alignment was fixed.
  if(Bone.Key==TEXT("pelvis")){Node->Node.Translation=FVector(0,0,-20);Node->Node.TranslationMode=BMM_Additive;Node->Node.TranslationSpace=BCS_ComponentSpace;}
  else{Node->Node.Rotation=FRotator(Bone.Value,0,0);Node->Node.RotationMode=BMM_Additive;Node->Node.RotationSpace=BCS_ComponentSpace;}
  Node->Node.AlphaInputType=EAnimAlphaInputType::Float;Node->Node.Alpha=0;
  Node->AllocateDefaultPins();
  if(!Connect(Previous,Node,TEXT("ComponentPose")))return false;
  if(auto* Alpha=Node->FindPin(TEXT("Alpha")))Graph->GetSchema()->TryCreateConnection(Getter->GetValuePin(),Alpha);
  else return false;
  Previous=Node;
 }
 auto* Back=Make(UAnimGraphNode_ComponentToLocalSpace::StaticClass(),X);
 auto* Root=Make(UAnimGraphNode_Root::StaticClass(),X+220);
 if(!Connect(Previous,Back)||!Connect(Back,Root))return false;
 FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
 FKismetEditorUtilities::CompileBlueprint(BP);
 BP->MarkPackageDirty();return BP->Status!=BS_Error;
#else
 return false;
#endif
}

FString UForestNPCSetupLibrary::BuildNavigationHelpers(UObject* Context)
{
#if WITH_EDITOR
 UWorld* W=Context?Context->GetWorld():nullptr;if(!W)return TEXT("No world");
 TArray<AActor*> Terrains;FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestSetup),false);
 for(TActorIterator<AActor> It(W);It;++It)
 {
  const FString Class=It->GetClass()->GetName();
  if(Class==TEXT("Landscape") || Class==TEXT("LandscapeStreamingProxy")){Terrains.Add(*It);Params.AddIgnoredActor(*It);}
  if(Cast<APawn>(*It))Params.AddIgnoredActor(*It);
  if(It->ActorHasTag(TEXT("NPC_WanderPoint")))
  {It->Modify();It->SetActorEnableCollision(false);Params.AddIgnoredActor(*It);It->MarkPackageDirty();}
  TArray<UStaticMeshComponent*> Meshes;It->GetComponents<UStaticMeshComponent>(Meshes);
  for(auto* Mesh:Meshes)
  {
   const FString Name=GetNameSafe(Mesh->GetStaticMesh());
   if(Name.StartsWith(TEXT("SM_Env_Fern_")) || Name.StartsWith(TEXT("SM_Env_Leaves_")) || Name.StartsWith(TEXT("SM_Env_Undergrowth_Fern_")) || Name.StartsWith(TEXT("SM_Env_Fern_Koru_")) || Name.StartsWith(TEXT("SM_Env_Mushroom_Small_Group_")) || Name.StartsWith(TEXT("SM_Env_Moss_Lumps_")) || Name.StartsWith(TEXT("SM_Env_Rock_Pebbles_")))
   {Mesh->Modify();Mesh->SetCollisionProfileName(TEXT("ForestDecoration"));Mesh->SetCanEverAffectNavigation(false);Params.AddIgnoredActor(*It);It->MarkPackageDirty();}
  }
 }
 auto Ground=[&](const FVector2D& XY,FVector& G)->bool
 {
  bool Found=false;FCollisionQueryParams P(SCENE_QUERY_STAT(ForestSetupGround),true);
  for(auto* Terrain:Terrains)
  {FHitResult H;if(Terrain->ActorLineTraceSingle(H,FVector(XY.X,XY.Y,10000),FVector(XY.X,XY.Y,-10000),ECC_Visibility,P) && (!Found || H.ImpactPoint.Z>G.Z)){G=H.ImpactPoint;Found=true;}}
  return Found;
 };
 auto Clear=[&](const FVector& G,float Half)->bool
 {return !W->OverlapBlockingTestByChannel(G+FVector(0,0,Half+4),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43,Half-1),Params);};
 auto FindNamed=[&](const FString& Label)->AActor*
 {for(TActorIterator<AActor> It(W);It;++It)if(It->GetActorLabel()==Label)return *It;return nullptr;};
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);
 int32 RoadCount=0,LinkCount=0,Rejected=0;
 TArray<AActor*> Roads;for(TActorIterator<AActor> It(W);It;++It)if(It->ActorHasTag(TEXT("NPC_RoadRouteDraft")))Roads.Add(*It);
 for(auto* Road:Roads)
 {
  auto* S=Road->FindComponentByClass<USplineComponent>();if(!S)continue;
  TArray<FVector> Samples;TArray<bool> Low,Stand;
  for(float D=0;D<=S->GetSplineLength();D+=50)
  {
   FVector G;if(!Ground(FVector2D(S->GetLocationAtDistanceAlongSpline(D,ESplineCoordinateSpace::World)),G))continue;
   Samples.Add(G);const bool CanStand=Clear(G,96);Stand.Add(CanStand);Low.Add(!CanStand && Clear(G,60));
   if(Samples.Num()%5==0)
   {
    const FString Label=FString::Printf(TEXT("NPC_Prefer_%s_%03d"),*Road->GetActorLabel(),Samples.Num()/5);
    auto* Area=Cast<AForestRoadPreference>(FindNamed(Label));if(!Area)Area=W->SpawnActor<AForestRoadPreference>();
    Area->Modify();Area->SetActorLabel(Label);Area->SetFolderPath(TEXT("NPC_Navigation/RoadPreference"));
    const FVector Tangent=S->GetTangentAtDistanceAlongSpline(D,ESplineCoordinateSpace::World);
    Area->Extent=FVector(150,180,160);Area->SetActorLocation(G+FVector(0,0,70));Area->SetActorRotation(FRotator(0,Tangent.Rotation().Yaw,0));Area->OnConstruction(Area->GetActorTransform());Area->MarkPackageDirty();++RoadCount;
   }
  }
  for(int32 I=1;I<Low.Num()-1;++I)
  {
   if(!Low[I] || !Stand[I-1])continue;
   int32 End=I;while(End<Low.Num() && Low[End])++End;
   if(End>=Low.Num() || !Stand[End] || End-I<2 || End-I>40){I=End;continue;}
   FVector Entry=Samples[I-1],Exit=Samples[End];FNavLocation A,B;
   if(!Nav || !Nav->ProjectPointToNavigation(Entry,A,FVector(100,100,90)) || !Nav->ProjectPointToNavigation(Exit,B,FVector(100,100,90))){++Rejected;I=End;continue;}
   bool Valid=true;
   for(int32 J=I;J<=End;++J)
   {
    const float H=FVector::Dist2D(Samples[J-1],Samples[J]);
    FHitResult Hit;
    if(FMath::Abs(Samples[J].Z-Samples[J-1].Z)>H*1.3 || W->SweepSingleByChannel(Hit,Samples[J-1]+FVector(0,0,64),Samples[J]+FVector(0,0,64),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43,59),Params)){Valid=false;break;}
   }
   if(!Valid){++Rejected;I=End;continue;}
   const FString Label=FString::Printf(TEXT("NPC_Crouch_%s_%03d"),*Road->GetActorLabel(),I);
   auto* Link=Cast<AForestCrouchPassage>(FindNamed(Label));if(!Link)Link=W->SpawnActor<AForestCrouchPassage>();
   Link->Modify();Link->SetActorLabel(Label);Link->SetFolderPath(TEXT("NPC_Navigation/CrouchPassages"));
   Link->SetActorLocation(A.Location);Link->PassagePath->ClearSplinePoints(false);
   Link->PassagePath->AddSplinePoint(A.Location,ESplineCoordinateSpace::World,false);
   for(int32 J=I;J<End;++J)Link->PassagePath->AddSplinePoint(Samples[J],ESplineCoordinateSpace::World,false);
   Link->PassagePath->AddSplinePoint(B.Location,ESplineCoordinateSpace::World,true);
   for(int32 J=0;J<Link->PassagePath->GetNumberOfSplinePoints();++J)Link->PassagePath->SetSplinePointType(J,ESplinePointType::Linear,false);
   Link->PassagePath->UpdateSpline();Link->RefreshLink();Link->MarkPackageDirty();++LinkCount;I=End;
  }
 }
 return FString::Printf(TEXT("road preference actors=%d; crouch passages=%d; rejected unsafe passages=%d"),RoadCount,LinkCount,Rejected);
#else
 return TEXT("Editor operation only");
#endif
}
FString UForestNPCSetupLibrary::CheckDestinationPaths(UObject* Context)
{
 auto* W=Context?Context->GetWorld():nullptr;if(!W)return TEXT("No world");
 TArray<AActor*> Markers;Markers.Init(nullptr,7);
 for(TActorIterator<AActor> It(W);It;++It)for(int32 I=0;I<7;++I)if(It->ActorHasTag(FName(*FString::Printf(TEXT("NPC_Destination%02d"),I+1))))Markers[I]=*It;
 for(TActorIterator<AActor> It(W);It;++It)for(int32 I=0;I<7;++I)if(It->ActorHasTag(FName(*FString::Printf(TEXT("NPC_Arrival%02d"),I+1))))Markers[I]=*It;
 FString Report;int32 Failed=0;
 for(int32 A=0;A<7;++A)for(int32 B=0;B<7;++B)
 {
  if(A==B)continue;
  auto Location=[](AActor* M)->FVector {if(!M)return FVector::ZeroVector;TArray<USceneComponent*> C;M->GetComponents<USceneComponent>(C);for(auto* P:C)if(P->ComponentHasTag(TEXT("NPC_Approach")))return P->GetComponentLocation();return M->GetActorLocation();};
  auto* Path=Markers[A] && Markers[B]?UNavigationSystemV1::FindPathToLocationSynchronously(W,Location(Markers[A]),Location(Markers[B]),nullptr,UForestRoadQueryFilter::StaticClass()):nullptr;
  if(!Path || !Path->IsValid() || Path->IsPartial()){Report+=FString::Printf(TEXT("NPC%02d->NPC%02d unavailable; "),A+1,B+1);++Failed;}
 }
 return FString::Printf(TEXT("%d/42 complete routes. %s"),42-Failed,*Report);
}

FString UForestNPCSetupLibrary::BuildGapPassage(UObject* Context,FVector Start,FVector End,const FString& Label)
{
#if WITH_EDITOR
 auto* W=Context?Context->GetWorld():nullptr;if(!W)return TEXT("No world");
 constexpr float Grid=20.f;
 TArray<AActor*> Terrains;FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestGap),false);
 for(TActorIterator<AActor> It(W);It;++It)
 {
  const FString Name=It->GetClass()->GetName();
  if(Name==TEXT("Landscape") || Name==TEXT("LandscapeStreamingProxy")){Terrains.Add(*It);Params.AddIgnoredActor(*It);}
  if(Cast<APawn>(*It) || It->ActorHasTag(TEXT("NPC_WanderPoint")))Params.AddIgnoredActor(*It);
 }
 auto Ground=[&](FIntPoint Cell,FVector& G,FVector& Normal)->bool
 {
  FCollisionQueryParams P(SCENE_QUERY_STAT(ForestGapFloor),true);
  for(auto* Terrain:Terrains)
  {
   FHitResult Hit;
   if(Terrain->ActorLineTraceSingle(Hit,FVector(Cell.X*Grid,Cell.Y*Grid,10000),FVector(Cell.X*Grid,Cell.Y*Grid,-10000),ECC_Visibility,P))
   {
    G=Hit.ImpactPoint;
    Normal=Hit.ImpactNormal;
    // Roads can continue over stairs or platforms above Landscape.
    FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(ForestGapWalkableFloor),false);
    for(TActorIterator<APawn> It(W);It;++It)FloorParams.AddIgnoredActor(*It);
    FHitResult Surface;
    const float Top=FMath::Max(Start.Z,End.Z)+200.f;
    const FVector2D Along(End-Start),Relative(G-Start);
    const float Fraction=FMath::Clamp(FVector2D::DotProduct(Relative,Along)/FMath::Max(1.f,Along.SizeSquared()),0.f,1.f);
    const float ExpectedZ=FMath::Lerp(Start.Z,End.Z,Fraction);
    if(W->LineTraceSingleByChannel(Surface,FVector(G.X,G.Y,Top),G-FVector(0,0,20),ECC_Pawn,FloorParams)
       && Surface.ImpactNormal.Z>=FMath::Cos(FMath::DegreesToRadians(55.f))
       && FMath::Abs(Surface.ImpactPoint.Z-ExpectedZ)<FMath::Abs(G.Z-ExpectedZ)) {G=Surface.ImpactPoint;Normal=Surface.ImpactNormal;}
    return true;
   }
  }
  return false;
 };
 struct FCell { FIntPoint XY;FVector Ground;float Cost=MAX_flt;int32 Parent=INDEX_NONE;bool Closed=false; };
 TArray<FCell> Nodes;TMap<FIntPoint,int32> Cache;
 const FIntPoint First(FMath::RoundToInt(Start.X/Grid),FMath::RoundToInt(Start.Y/Grid));
 const FIntPoint Last(FMath::RoundToInt(End.X/Grid),FMath::RoundToInt(End.Y/Grid));
 auto NodeAt=[&](FIntPoint Cell)->int32
 {
  if(auto* Known=Cache.Find(Cell))return *Known;
  if(Cell.X<FMath::Min(First.X,Last.X)-50 || Cell.X>FMath::Max(First.X,Last.X)+50 || Cell.Y<FMath::Min(First.Y,Last.Y)-50 || Cell.Y>FMath::Max(First.Y,Last.Y)+50)return INDEX_NONE;
  FVector G,Normal;
  if(!Ground(Cell,G,Normal)){Cache.Add(Cell,INDEX_NONE);return INDEX_NONE;}
  if(W->OverlapBlockingTestByChannel(G+FVector(0,0,64),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43,59),Params))
  {
   FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(ForestGapFloorSurface),false);
   for(TActorIterator<APawn> It(W);It;++It)FloorParams.AddIgnoredActor(*It);
   FHitResult Floor;
   if(!W->LineTraceSingleByChannel(Floor,G+FVector(0,0,1000),G-FVector(0,0,20),ECC_Pawn,FloorParams) || Floor.ImpactNormal.Z<FMath::Cos(FMath::DegreesToRadians(55.f)))
   {Cache.Add(Cell,INDEX_NONE);return INDEX_NONE;}
   G=Floor.ImpactPoint;
   Normal=Floor.ImpactNormal;
   if(W->OverlapBlockingTestByChannel(G+FVector(0,0,64),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43,59),Params))
   {Cache.Add(Cell,INDEX_NONE);return INDEX_NONE;}
  }
  if(Normal.Z<FMath::Cos(FMath::DegreesToRadians(55.f))){Cache.Add(Cell,INDEX_NONE);return INDEX_NONE;}
  const int32 I=Nodes.AddDefaulted();Nodes[I].XY=Cell;Nodes[I].Ground=G;Cache.Add(Cell,I);return I;
 };
 auto Nearest=[&](FIntPoint Cell,float DesiredZ)->int32
 {
  int32 Best=INDEX_NONE;float Distance=MAX_flt;
  for(int32 X=-6;X<=6;++X)for(int32 Y=-6;Y<=6;++Y)
  {
   const int32 I=NodeAt(Cell+FIntPoint(X,Y));if(I==INDEX_NONE)continue;
   const float D=FMath::Square(X)+FMath::Square(Y)+FMath::Square((Nodes[I].Ground.Z-DesiredZ)/Grid);
   if(D<Distance){Distance=D;Best=I;}
  }
  return Best;
 };
 const int32 Begin=Nearest(First,Start.Z),Finish=Nearest(Last,End.Z);
 if(Begin==INDEX_NONE || Finish==INDEX_NONE)return TEXT("Endpoint lacks physical crouch clearance");
 Nodes[Begin].Cost=0;TArray<int32> Open{Begin};bool Found=false;
 for(int32 Pass=0;Pass<50000 && !Open.IsEmpty();++Pass)
 {
  int32 At=0;float Best=MAX_flt;
  for(int32 I=0;I<Open.Num();++I)
  {
   const auto& N=Nodes[Open[I]];
   const float Score=N.Cost+FVector::Dist2D(N.Ground,Nodes[Finish].Ground);
   if(Score<Best){Best=Score;At=I;}
  }
  const int32 Current=Open[At];Open.RemoveAtSwap(At);
  if(Nodes[Current].Closed)continue;
  Nodes[Current].Closed=true;
  if(Current==Finish){Found=true;break;}
  const FIntPoint XY=Nodes[Current].XY;const FVector G=Nodes[Current].Ground;
  for(int32 X=-1;X<=1;++X)for(int32 Y=-1;Y<=1;++Y)
  {
   if(X==0 && Y==0)continue;
   const int32 Next=NodeAt(XY+FIntPoint(X,Y));
   if(Next==INDEX_NONE || Nodes[Next].Closed)continue;
   const FVector NG=Nodes[Next].Ground;const float H=FVector::Dist2D(G,NG);
   if(FMath::Abs(G.Z-NG.Z)>H*1.3f)continue;
   FHitResult Hit;
   if(W->SweepSingleByChannel(Hit,G+FVector(0,0,64),NG+FVector(0,0,64),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43,59),Params))
   {
    // CharacterMovement can step over a riser up to 45cm.
    if(FMath::Abs(G.Z-NG.Z)>45.f || W->SweepSingleByChannel(Hit,G+FVector(0,0,109),NG+FVector(0,0,109),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43,59),Params))continue;
   }
   const float NewCost=Nodes[Current].Cost+H;
   if(NewCost<Nodes[Next].Cost){Nodes[Next].Cost=NewCost;Nodes[Next].Parent=Current;Open.Add(Next);}
  }
 }
 if(!Found)return FString::Printf(TEXT("No collision-safe crouch corridor (%d tested cells)"),Cache.Num());
 TArray<FVector> Reverse;for(int32 I=Finish;I!=INDEX_NONE;I=Nodes[I].Parent)Reverse.Add(Nodes[I].Ground);
 TArray<FVector> Points;Points.Add(Start);
 for(int32 I=Reverse.Num()-1;I>=0;--I)Points.Add(Reverse[I]);
 Points.Add(End);
 // Greedily simplify only where a real crouched capsule sweep is clear.
 TArray<FVector> Simple{Points[0]};int32 At=0;
 while(At<Points.Num()-1)
 {
  int32 Next=At+1;
  for(int32 J=At+2;J<Points.Num() && J<=At+10;++J)
  {
   FHitResult Hit;
   if(FMath::Abs(Points[J].Z-Points[At].Z)>FVector::Dist2D(Points[J],Points[At])*1.3 ||
    W->SweepSingleByChannel(Hit,Points[At]+FVector(0,0,64),Points[J]+FVector(0,0,64),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(43,59),Params))break;
   Next=J;
  }
  Simple.Add(Points[Next]);At=Next;
 }
 AForestCrouchPassage* Link=nullptr;
 for(TActorIterator<AForestCrouchPassage> It(W);It;++It)if(It->GetActorLabel()==Label)Link=*It;
 if(!Link)Link=W->SpawnActor<AForestCrouchPassage>();
 Link->Modify();Link->SetActorLabel(Label);Link->SetFolderPath(TEXT("NPC_Navigation/CrouchPassages"));
 Link->SetActorLocation(Start);Link->PassagePath->ClearSplinePoints(false);
 for(const auto& Point:Simple)Link->PassagePath->AddSplinePoint(Point,ESplineCoordinateSpace::World,false);
 for(int32 I=0;I<Simple.Num();++I)Link->PassagePath->SetSplinePointType(I,ESplinePointType::Linear,false);
 Link->PassagePath->UpdateSpline();Link->RefreshLink();Link->MarkPackageDirty();
 return FString::Printf(TEXT("Created %s: %d safe path points, %.0f cm corridor"),*Label,Simple.Num(),Nodes[Finish].Cost);
#else
 return TEXT("Editor operation only");
#endif
}


FString UForestNPCSetupLibrary::CreateArrivalPoints(UObject* Context)
{
#if WITH_EDITOR
 auto* W=Context?Context->GetWorld():nullptr;if(!W)return TEXT("No world");
 auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(W);if(!Nav)return TEXT("No navigation");
 TArray<AActor*> Terrains,Markers;FCollisionQueryParams Params(SCENE_QUERY_STAT(ForestArrival),false);
 for(TActorIterator<AActor> It(W);It;++It)
 {
  const FString Class=It->GetClass()->GetName();
  if(Class==TEXT("Landscape") || Class==TEXT("LandscapeStreamingProxy"))Terrains.Add(*It);
  if(Cast<APawn>(*It) || It->ActorHasTag(TEXT("NPC_WanderPoint")))Params.AddIgnoredActor(*It);
  if(It->ActorHasTag(TEXT("NPC_WanderPoint")))Markers.Add(*It);
 }
 FString Report;
 for(auto* Marker:Markers)
 {
  int32 Number=0;
  for(int32 I=1;I<=7;++I)if(Marker->ActorHasTag(FName(*FString::Printf(TEXT("NPC_Destination%02d"),I))))Number=I;
  if(!Number)continue;
  ATargetPoint* ManualArrival=nullptr;
  const FName ArrivalTag(*FString::Printf(TEXT("NPC_Arrival%02d"),Number));
  for(TActorIterator<ATargetPoint> It(W);It;++It)
   if(It->ActorHasTag(ArrivalTag) && It->ActorHasTag(TEXT("NPC_ManualArrival")))ManualArrival=*It;
  if(ManualArrival)
  {Report+=FString::Printf(TEXT("NPC%02d: preserved editable manual arrival; "),Number);continue;}
  const FVector M=Marker->GetActorLocation();FVector Best=FVector::ZeroVector;bool Found=false;
  for(float Radius:{0.f,80.f,160.f,200.f})
  {
   for(int32 Direction=0;Direction<(Radius==0?1:16);++Direction)
   {
    const float Angle=Direction*2*PI/16;
    const FVector2D XY(M.X+FMath::Cos(Angle)*Radius,M.Y+FMath::Sin(Angle)*Radius);
    FVector Ground;bool GotGround=false;FCollisionQueryParams P(SCENE_QUERY_STAT(ForestArrivalGround),true);
    for(auto* Terrain:Terrains)
    {
     FHitResult Hit;if(Terrain->ActorLineTraceSingle(Hit,FVector(XY.X,XY.Y,10000),FVector(XY.X,XY.Y,-10000),ECC_Visibility,P))
     {Ground=Hit.ImpactPoint;GotGround=true;break;}
    }
    if(!GotGround)continue;
    FHitResult Surface;
    if(W->LineTraceSingleByChannel(Surface,FVector(XY.X,XY.Y,M.Z+200),Ground-FVector(0,0,20),ECC_Pawn,Params)
       && Surface.ImpactNormal.Z>=FMath::Cos(FMath::DegreesToRadians(55.f))) Ground=Surface.ImpactPoint;
    FNavLocation Projected;
    if(!Nav->ProjectPointToNavigation(Ground+FVector(0,0,20),Projected,FVector(60,60,90)))continue;
    if(FVector::Dist2D(Projected.Location,M)>220)continue;
    if(W->OverlapBlockingTestByChannel(Projected.Location+FVector(0,0,100),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,94),Params))continue;
    Best=Projected.Location;Found=true;break;
   }
   if(Found)break;
  }
  if(!Found){Report+=FString::Printf(TEXT("NPC%02d: no safe standing arrival within 2m; "),Number);continue;}
  const FName Tag(*FString::Printf(TEXT("NPC_Arrival%02d"),Number));
  ATargetPoint* Arrival=nullptr;
  for(TActorIterator<ATargetPoint> It(W);It;++It)if(It->ActorHasTag(Tag))Arrival=*It;
  if(!Arrival)Arrival=W->SpawnActor<ATargetPoint>();
  Arrival->Modify();Arrival->Tags.AddUnique(Tag);
  Arrival->SetActorLabel(FString::Printf(TEXT("NPC%02d_Arrival"),Number));
  Arrival->SetFolderPath(TEXT("NPC_Navigation/ArrivalPoints"));Arrival->SetActorLocation(Best);
  Arrival->MarkPackageDirty();
  Report+=FString::Printf(TEXT("NPC%02d=(%.0f,%.0f,%.0f); "),Number,Best.X,Best.Y,Best.Z);
 }
 return Report;
#else
 return TEXT("Editor operation only");
#endif
}

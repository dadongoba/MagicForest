#include "ForestMajorArt.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
    struct FMajorArtFile
    {
        const TCHAR* CardId;
        const TCHAR* Filename;
    };

    // This list mirrors the ID -> file mapping in ArtSource/ThothMajorArcana/major_arcana_manifest.json.
    constexpr FMajorArtFile Files[] = {
        {TEXT("0"), TEXT("Major_00_Fool.png")},
        {TEXT("I"), TEXT("Major_01_Magician.png")},
        {TEXT("II"), TEXT("Major_02_HighPriestess.png")},
        {TEXT("III"), TEXT("Major_03_Empress.png")},
        {TEXT("IV"), TEXT("Major_04_Emperor.png")},
        {TEXT("V"), TEXT("Major_05_Hierophant.png")},
        {TEXT("VI"), TEXT("Major_06_Lovers.png")},
        {TEXT("VII"), TEXT("Major_07_Chariot.png")},
        {TEXT("VIII"), TEXT("Major_08_Adjustment.png")},
        {TEXT("IX"), TEXT("Major_09_Hermit.png")},
        {TEXT("X"), TEXT("Major_10_Fortune.png")},
        {TEXT("XI"), TEXT("Major_11_Lust.png")},
        {TEXT("XII"), TEXT("Major_12_HangedMan.png")},
        {TEXT("XIII"), TEXT("Major_13_Death.png")},
        {TEXT("XIV"), TEXT("Major_14_Art.png")},
        {TEXT("XV"), TEXT("Major_15_Devil.png")},
        {TEXT("XVI"), TEXT("Major_16_Tower.png")},
        {TEXT("XVII"), TEXT("Major_17_Star.png")},
        {TEXT("XVIII"), TEXT("Major_18_Moon.png")},
        {TEXT("XIX"), TEXT("Major_19_Sun.png")},
        {TEXT("XX"), TEXT("Major_20_Aeon.png")},
        {TEXT("XXI"), TEXT("Major_21_Universe.png")},
    };

    // Slate keeps a pointer to each brush. This process-lifetime cache owns every brush it returns.
    const TMap<FName, TSharedPtr<FSlateDynamicImageBrush>>& GetBrushes()
    {
        static const TMap<FName, TSharedPtr<FSlateDynamicImageBrush>> Brushes = []
        {
            TMap<FName, TSharedPtr<FSlateDynamicImageBrush>> Result;
            const FString ArtDir = FPaths::ConvertRelativePathToFull(
                FPaths::ProjectContentDir() / TEXT("UI/Textures/MajorArcana"));
            int32 Missing = 0;
            for (const FMajorArtFile& File : Files)
            {
                const FString Path = ArtDir / File.Filename;
                if (!IFileManager::Get().FileExists(*Path))
                {
                    ++Missing;
                    continue;
                }
                Result.Add(FName(File.CardId), MakeShared<FSlateDynamicImageBrush>(
                    FName(*Path), FVector2D(142.0, 512.0)));
            }
            if (Missing > 0)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("Forest major art: %d of 22 card PNGs missing from %s; affected cards show no image."),
                    Missing, *ArtDir);
            }
            return Result;
        }();
        return Brushes;
    }
}

const FSlateBrush* ForestMajorArt::BrushFor(FName CardId)
{
    const TSharedPtr<FSlateDynamicImageBrush>* Found = GetBrushes().Find(CardId);
    return Found ? Found->Get() : nullptr;
}

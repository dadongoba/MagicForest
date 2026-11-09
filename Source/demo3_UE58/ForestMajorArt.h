#pragma once

#include "CoreMinimal.h"

struct FSlateBrush;

/** Runtime artwork for the 22 daily major cards. Keep card IDs aligned with MakeMajorCards(). */
namespace ForestMajorArt
{
    constexpr float AspectRatio = 142.0f / 512.0f;

    /** Stable Slate brush for the given card ID (0, I ... XXI), or nullptr if art is unavailable. */
    const FSlateBrush* BrushFor(FName CardId);
}

#include "StageG0/StageG0Types.h"

FString StageGVisualDirectionName(EStageGVisualDirection Direction)
{
    switch (Direction)
    {
    case EStageGVisualDirection::Front: return TEXT("Front");
    case EStageGVisualDirection::FrontRight: return TEXT("FrontRight");
    case EStageGVisualDirection::Right: return TEXT("Right");
    case EStageGVisualDirection::BackRight: return TEXT("BackRight");
    case EStageGVisualDirection::Back: return TEXT("Back");
    case EStageGVisualDirection::BackLeft: return TEXT("BackLeft");
    case EStageGVisualDirection::Left: return TEXT("Left");
    case EStageGVisualDirection::FrontLeft: return TEXT("FrontLeft");
    }
    return TEXT("Front");
}

FString StageGVisualDirectionSuffix(EStageGVisualDirection Direction)
{
    switch (Direction)
    {
    case EStageGVisualDirection::Front: return TEXT("F");
    case EStageGVisualDirection::FrontRight: return TEXT("FR");
    case EStageGVisualDirection::Right: return TEXT("R");
    case EStageGVisualDirection::BackRight: return TEXT("BR");
    case EStageGVisualDirection::Back: return TEXT("B");
    case EStageGVisualDirection::BackLeft: return TEXT("BL");
    case EStageGVisualDirection::Left: return TEXT("L");
    case EStageGVisualDirection::FrontLeft: return TEXT("FL");
    }
    return TEXT("F");
}

FString StageGVisualActionName(EStageGVisualAction Action)
{
    switch (Action)
    {
    case EStageGVisualAction::Idle: return TEXT("IDLE");
    case EStageGVisualAction::Walk: return TEXT("WALK");
    case EStageGVisualAction::Talk: return TEXT("TALK");
    case EStageGVisualAction::Warn: return TEXT("WARN");
    case EStageGVisualAction::Report: return TEXT("REPORT");
    case EStageGVisualAction::Arrest: return TEXT("ARREST");
    case EStageGVisualAction::Flee: return TEXT("FLEE");
    case EStageGVisualAction::MonsterMove: return TEXT("MOVE");
    case EStageGVisualAction::MonsterCharge: return TEXT("CHARGE");
    case EStageGVisualAction::MonsterBite: return TEXT("BITE");
    case EStageGVisualAction::Hit: return TEXT("HIT");
    case EStageGVisualAction::Death: return TEXT("DEATH");
    }
    return TEXT("IDLE");
}

FString StageGVisualDirectionJapanese(EStageGVisualDirection Direction)
{
    switch (Direction)
    {
    case EStageGVisualDirection::Front: return TEXT("正面");
    case EStageGVisualDirection::FrontRight: return TEXT("右前");
    case EStageGVisualDirection::Right: return TEXT("右");
    case EStageGVisualDirection::BackRight: return TEXT("右後");
    case EStageGVisualDirection::Back: return TEXT("背面");
    case EStageGVisualDirection::BackLeft: return TEXT("左後");
    case EStageGVisualDirection::Left: return TEXT("左");
    case EStageGVisualDirection::FrontLeft: return TEXT("左前");
    }
    return TEXT("正面");
}

FString StageGVisualActionJapanese(EStageGVisualAction Action)
{
    switch (Action)
    {
    case EStageGVisualAction::Idle: return TEXT("待機");
    case EStageGVisualAction::Walk: return TEXT("歩行");
    case EStageGVisualAction::Talk: return TEXT("会話");
    case EStageGVisualAction::Warn: return TEXT("警告");
    case EStageGVisualAction::Report: return TEXT("報告");
    case EStageGVisualAction::Arrest: return TEXT("拘束");
    case EStageGVisualAction::Flee: return TEXT("逃走");
    case EStageGVisualAction::MonsterMove: return TEXT("移動");
    case EStageGVisualAction::MonsterCharge: return TEXT("突進");
    case EStageGVisualAction::MonsterBite: return TEXT("噛みつき");
    case EStageGVisualAction::Hit: return TEXT("被弾");
    case EStageGVisualAction::Death: return TEXT("死亡");
    }
    return TEXT("待機");
}

FString StageGVisualFamilyJapanese(const FString& AssetFamily)
{
    const FString Family = AssetFamily.ToUpper();
    if (Family == TEXT("PLAYER")) return TEXT("プレイヤー");
    if (Family == TEXT("GUARD")) return TEXT("衛兵");
    if (Family == TEXT("CAPTAIN")) return TEXT("隊長");
    if (Family == TEXT("BROKER")) return TEXT("商人");
    if (Family == TEXT("FANG_RAT")) return TEXT("牙鼠");
    return TEXT("一般住民");
}

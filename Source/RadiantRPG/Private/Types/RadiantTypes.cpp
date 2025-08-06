// Private/Types/RadiantTypes.cpp
// Implementation of utility functions for RadiantRPG core types

#include "Types/RadiantTypes.h"
#include "GameplayTagsManager.h"
#include "Types/SkillTypes.h"
#include "Types/WorldTypes.h"

float URadiantTypeUtils::TimeOfDayToHour(ETimeOfDay TimeOfDay)
{
    switch (TimeOfDay)
    {
        case ETimeOfDay::Dawn:      return 6.0f;
        case ETimeOfDay::Morning:   return 9.0f;
        case ETimeOfDay::Noon:      return 12.0f;
        case ETimeOfDay::Midday: return 15.0f;
        case ETimeOfDay::Dusk:      return 18.0f;
        case ETimeOfDay::Evening:   return 20.0f;
        case ETimeOfDay::Night:     return 22.0f;
        case ETimeOfDay::Midnight:  return 0.0f;
        default:                    return 12.0f;
    }
}

ETimeOfDay URadiantTypeUtils::HourToTimeOfDay(float Hour)
{
    // Normalize hour to 0-24 range
    Hour = FMath::Fmod(Hour, 24.0f);
    if (Hour < 0.0f) Hour += 24.0f;

    if (Hour >= 23.0f || Hour < 2.0f)
        return ETimeOfDay::Midnight;
    else if (Hour >= 2.0f && Hour < 7.0f)
        return ETimeOfDay::Dawn;
    else if (Hour >= 7.0f && Hour < 11.0f)
        return ETimeOfDay::Morning;
    else if (Hour >= 11.0f && Hour < 14.0f)
        return ETimeOfDay::Noon;
    else if (Hour >= 14.0f && Hour < 17.0f)
        return ETimeOfDay::Midday;
    else if (Hour >= 17.0f && Hour < 19.0f)
        return ETimeOfDay::Dusk;
    else if (Hour >= 19.0f && Hour < 21.0f)
        return ETimeOfDay::Evening;
    else
        return ETimeOfDay::Night;
}

FText URadiantTypeUtils::GetWeatherDisplayName(EWeatherType WeatherType)
{
    switch (WeatherType)
    {
        case EWeatherType::Clear:           return NSLOCTEXT("Weather", "Clear", "Clear");
        case EWeatherType::Cloudy:          return NSLOCTEXT("Weather", "Cloudy", "Cloudy");
        case EWeatherType::Fog:             return NSLOCTEXT("Weather", "Fog", "Foggy");
        case EWeatherType::LightRain:       return NSLOCTEXT("Weather", "LightRain", "Light Rain");
        case EWeatherType::HeavyRain:       return NSLOCTEXT("Weather", "HeavyRain", "Heavy Rain");
        case EWeatherType::Thunderstorm:    return NSLOCTEXT("Weather", "Thunderstorm", "Thunderstorm");
        case EWeatherType::Snow:            return NSLOCTEXT("Weather", "Snow", "Snow");
        case EWeatherType::Blizzard:        return NSLOCTEXT("Weather", "Blizzard", "Blizzard");
        case EWeatherType::Sandstorm:       return NSLOCTEXT("Weather", "Sandstorm", "Sandstorm");
        default:                            return NSLOCTEXT("Weather", "Unknown", "Unknown");
    }
}

FText URadiantTypeUtils::GetRelationshipDisplayName(EFactionRelationship Relationship)
{
    switch (Relationship)
    {
        case EFactionRelationship::Enemy:       return NSLOCTEXT("Faction", "Enemy", "Enemy");
        case EFactionRelationship::Hostile:     return NSLOCTEXT("Faction", "Hostile", "Hostile");
        case EFactionRelationship::Unfriendly:  return NSLOCTEXT("Faction", "Unfriendly", "Unfriendly");
        case EFactionRelationship::Neutral:     return NSLOCTEXT("Faction", "Neutral", "Neutral");
        case EFactionRelationship::Friendly:    return NSLOCTEXT("Faction", "Friendly", "Friendly");
        case EFactionRelationship::Allied:      return NSLOCTEXT("Faction", "Allied", "Allied");
        default:                               return NSLOCTEXT("Faction", "Unknown", "Unknown");
    }
}

EFactionRelationship URadiantTypeUtils::ReputationToRelationship(float ReputationValue)
{
    // Convert -100 to 100 reputation scale to relationship enum
    if (ReputationValue <= -75.0f)
        return EFactionRelationship::Enemy;
    else if (ReputationValue <= -25.0f)
        return EFactionRelationship::Hostile;
    else if (ReputationValue <= -5.0f)
        return EFactionRelationship::Unfriendly;
    else if (ReputationValue <= 5.0f)
        return EFactionRelationship::Neutral;
    else if (ReputationValue <= 25.0f)
        return EFactionRelationship::Friendly;
    else
        return EFactionRelationship::Allied;
}

FText URadiantTypeUtils::GetSkillCategoryDisplayName(ESkillCategory Category)
{
    switch (Category)
    {
        case ESkillCategory::Combat:        return NSLOCTEXT("Skill", "Combat", "Combat");
        case ESkillCategory::Magic:         return NSLOCTEXT("Skill", "Magic", "Magic");
        case ESkillCategory::Crafting:      return NSLOCTEXT("Skill", "Crafting", "Crafting");
        case ESkillCategory::Survival:      return NSLOCTEXT("Skill", "Survival", "Survival");
        case ESkillCategory::Social:        return NSLOCTEXT("Skill", "Social", "Social");
        case ESkillCategory::Movement:      return NSLOCTEXT("Skill", "Movement", "Movement");
        case ESkillCategory::Knowledge:     return NSLOCTEXT("Skill", "Knowledge", "Knowledge");
        default:                          return NSLOCTEXT("Skill", "Unknown", "Unknown");
    }
}

bool URadiantTypeUtils::IsSkillInCategory(FGameplayTag SkillTag, ESkillCategory Category)
{
    if (!SkillTag.IsValid())
        return false;

    // Build category tag string
    FString CategoryString;
    switch (Category)
    {
        case ESkillCategory::Combat:        CategoryString = "Skill.Combat"; break;
        case ESkillCategory::Magic:         CategoryString = "Skill.Magic"; break;
        case ESkillCategory::Crafting:      CategoryString = "Skill.Crafting"; break;
        case ESkillCategory::Survival:      CategoryString = "Skill.Survival"; break;
        case ESkillCategory::Social:        CategoryString = "Skill.Social"; break;
        case ESkillCategory::Movement:      CategoryString = "Skill.Movement"; break;
        case ESkillCategory::Knowledge:     CategoryString = "Skill.Knowledge"; break;
        default:                           return false;
    }

    // Check if skill tag matches the category
    FGameplayTag CategoryTag = FGameplayTag::RequestGameplayTag(FName(*CategoryString), false);
    if (CategoryTag.IsValid())
    {
        return SkillTag.MatchesTag(CategoryTag);
    }

    return false;
}
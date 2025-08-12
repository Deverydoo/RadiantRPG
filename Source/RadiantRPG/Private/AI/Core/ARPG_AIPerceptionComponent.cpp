// Source/RadiantRPG/Private/AI/Core/ARPG_AIPerceptionComponent.cpp

#include "AI/Core/ARPG_AIPerceptionComponent.h"
#include "AI/Core/ARPG_AIBrainComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Touch.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Touch.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UARPG_AIPerceptionComponent::UARPG_AIPerceptionComponent()
{
    PerceptionConfig = FARPG_PerceptionConfiguration();
    PrimaryComponentTick.bCanEverTick = false;
    
    // Create sense configs in constructor - NOT in BeginPlay/InitializeSenseConfigurations
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    TouchConfig = CreateDefaultSubobject<UAISenseConfig_Touch>(TEXT("TouchConfig"));
    
    UE_LOG(LogTemp, Log, TEXT("AIPerceptionComponent: Component created"));
}

void UARPG_AIPerceptionComponent::BeginPlay()
{
    Super::BeginPlay();
    
    InitializeBrainReference();
    InitializeSenseConfigurations();
    SetupPerceptionEvents();
    
    UE_LOG(LogTemp, Log, TEXT("AIPerceptionComponent: Initialized for %s"), 
           GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"));
}

void UARPG_AIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    OnPerceptionUpdated.Clear();
    OnTargetPerceptionUpdated.Clear();
    
    Super::EndPlay(EndPlayReason);
}

void UARPG_AIPerceptionComponent::InitializePerception(const FARPG_PerceptionConfiguration& Config)
{
    PerceptionConfig = Config;
    InitializeSenseConfigurations();
    
    if (PerceptionConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Perception initialized with custom configuration"));
    }
}

void UARPG_AIPerceptionComponent::UpdatePerceptionConfiguration(const FARPG_PerceptionConfiguration& NewConfig)
{
    PerceptionConfig = NewConfig;
    
    if (SightConfig && PerceptionConfig.bEnableSight)
    {
        SightConfig->SightRadius = PerceptionConfig.SightRadius;
        SightConfig->PeripheralVisionAngleDegrees = PerceptionConfig.SightAngle;
        SightConfig->SetMaxAge(PerceptionConfig.SightMaxAge);
    }
    
    if (HearingConfig && PerceptionConfig.bEnableHearing)
    {
        HearingConfig->HearingRange = PerceptionConfig.HearingRadius;
    }
    
    if (PerceptionConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Perception configuration updated"));
    }
}

TArray<AActor*> UARPG_AIPerceptionComponent::GetCurrentlyPerceivedActors() const
{
    TArray<AActor*> PerceivedActors;
    GetKnownPerceivedActors(nullptr, PerceivedActors);
    return PerceivedActors;
}

TArray<AActor*> UARPG_AIPerceptionComponent::GetActorsPerceivedBySense(TSubclassOf<UAISense> SenseClass) const
{
    TArray<AActor*> PerceivedActors;
    GetKnownPerceivedActors(SenseClass, PerceivedActors);
    return PerceivedActors;
}

bool UARPG_AIPerceptionComponent::IsActorPerceived(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return false;
    }
    
    FActorPerceptionBlueprintInfo Info;
    return const_cast<UARPG_AIPerceptionComponent*>(this)->GetActorsPerception(Actor, Info);
}

FVector UARPG_AIPerceptionComponent::GetLastKnownLocation(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return FVector::ZeroVector;
    }
    
    FActorPerceptionBlueprintInfo Info;
    if (const_cast<UARPG_AIPerceptionComponent*>(this)->GetActorsPerception(Actor, Info) && Info.LastSensedStimuli.Num() > 0)
    {
        return Info.LastSensedStimuli[0].StimulusLocation;
    }
    
    return Actor->GetActorLocation();
}

bool UARPG_AIPerceptionComponent::CanSeeLocation(FVector Location) const
{
    if (!SightConfig || !PerceptionConfig.bEnableSight)
    {
        return false;
    }
    
    AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return false;
    }
    
    FVector OwnerLocation = Owner->GetActorLocation();
    FVector Direction = (Location - OwnerLocation).GetSafeNormal();
    float Distance = FVector::Dist(OwnerLocation, Location);
    
    if (Distance > PerceptionConfig.SightRadius)
    {
        return false;
    }
    
    FVector OwnerForward = Owner->GetActorForwardVector();
    float AngleDot = FVector::DotProduct(OwnerForward, Direction);
    float MaxAngleCos = FMath::Cos(FMath::DegreesToRadians(PerceptionConfig.SightAngle * 0.5f));
    
    if (AngleDot < MaxAngleCos)
    {
        return false;
    }
    
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Owner);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, OwnerLocation, Location, ECC_Visibility, QueryParams))
    {
        return false;
    }
    
    return true;
}

void UARPG_AIPerceptionComponent::AddManualStimulus(const FARPG_AIStimulus& Stimulus)
{
    if (!BrainComponent.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot add manual stimulus - no brain component"));
        return;
    }
    
    BrainComponent->ProcessStimulus(Stimulus);
    OnPerceptionStimulus.Broadcast(Stimulus, Stimulus.SourceActor.Get());
    
    if (PerceptionConfig.bEnableDebugLogging)
    {
        UE_LOG(LogTemp, Log, TEXT("Manual stimulus added: %s"), *Stimulus.StimulusTag.ToString());
    }
}

void UARPG_AIPerceptionComponent::GenerateSoundStimulus(FVector SoundLocation, float Loudness, FGameplayTag SoundTag)
{
    FARPG_AIStimulus Stimulus;
    Stimulus.StimulusType = EARPG_StimulusType::Audio;
    Stimulus.StimulusTag = SoundTag.IsValid() ? SoundTag : FGameplayTag::RequestGameplayTag(TEXT("AI.Perception.Sound.Generic"));
    Stimulus.Intensity = FMath::Clamp(Loudness, 0.0f, 1.0f);
    Stimulus.Location = SoundLocation;
    Stimulus.Timestamp = GetWorld()->GetTimeSeconds();
    
    float Distance = FVector::Dist(GetOwner()->GetActorLocation(), SoundLocation);
    if (Distance <= PerceptionConfig.HearingRadius)
    {
        float DistanceFactor = 1.0f - (Distance / PerceptionConfig.HearingRadius);
        Stimulus.Intensity *= DistanceFactor;
        AddManualStimulus(Stimulus);
    }
}

void UARPG_AIPerceptionComponent::GenerateVisualStimulus(AActor* VisualActor, float Intensity, FGameplayTag VisualTag)
{
    if (!IsValid(VisualActor))
    {
        return;
    }
    
    FARPG_AIStimulus Stimulus;
    Stimulus.StimulusType = EARPG_StimulusType::Visual;
    Stimulus.StimulusTag = VisualTag.IsValid() ? VisualTag : FGameplayTag::RequestGameplayTag(TEXT("AI.Perception.Sight.Generic"));
    Stimulus.Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    Stimulus.Location = VisualActor->GetActorLocation();
    Stimulus.SourceActor = VisualActor;
    Stimulus.Timestamp = GetWorld()->GetTimeSeconds();
    
    if (CanSeeLocation(VisualActor->GetActorLocation()))
    {
        AddManualStimulus(Stimulus);
    }
}

void UARPG_AIPerceptionComponent::InitializeBrainReference()
{
    if (AActor* Owner = GetOwner())
    {
        BrainComponent = Owner->FindComponentByClass<UARPG_AIBrainComponent>();
        if (!BrainComponent.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("AIPerceptionComponent: No brain component found on %s"), *Owner->GetName());
        }
    }
}

void UARPG_AIPerceptionComponent::InitializeSenseConfigurations()
{
    // Configure existing sense configs created in constructor
    if (PerceptionConfig.bEnableSight && SightConfig)
    {
        SightConfig->SightRadius = PerceptionConfig.SightRadius;
        SightConfig->LoseSightRadius = PerceptionConfig.SightRadius + 100.0f;
        SightConfig->PeripheralVisionAngleDegrees = PerceptionConfig.SightAngle;
        SightConfig->SetMaxAge(PerceptionConfig.SightMaxAge);
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        
        ConfigureSense(*SightConfig);
    }
    
    if (PerceptionConfig.bEnableHearing && HearingConfig)
    {
        HearingConfig->HearingRange = PerceptionConfig.HearingRadius;
        HearingConfig->SetMaxAge(PerceptionConfig.HearingMaxAge);
        HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
        HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
        HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
        
        ConfigureSense(*HearingConfig);
    }
    
    if (PerceptionConfig.bEnableTouch && TouchConfig)
    {
        TouchConfig->SetMaxAge(PerceptionConfig.TouchMaxAge);
        TouchConfig->DetectionByAffiliation.bDetectNeutrals = true;
        TouchConfig->DetectionByAffiliation.bDetectFriendlies = true;
        TouchConfig->DetectionByAffiliation.bDetectEnemies = true;
        
        ConfigureSense(*TouchConfig);
    }
}

void UARPG_AIPerceptionComponent::SetupPerceptionEvents()
{
    OnPerceptionUpdated.AddDynamic(this, &UARPG_AIPerceptionComponent::HandlePerceptionUpdated);
    OnTargetPerceptionUpdated.AddDynamic(this, &UARPG_AIPerceptionComponent::HandleTargetPerceptionUpdated);
}

FARPG_AIStimulus UARPG_AIPerceptionComponent::ConvertPerceptionToStimulus(const FActorPerceptionUpdateInfo& UpdateInfo)
{
    FARPG_AIStimulus Stimulus;
    
    if (UpdateInfo.Stimulus.Type.IsValid())
    {
        Stimulus.StimulusType = GetStimulusTypeFromSenseID(UpdateInfo.Stimulus.Type);
    }
    else
    {
        Stimulus.StimulusType = EARPG_StimulusType::Visual;
    }
    
    Stimulus.Intensity = CalculateStimulusIntensity(UpdateInfo);
    Stimulus.Location = UpdateInfo.Stimulus.StimulusLocation;
    Stimulus.SourceActor = UpdateInfo.Target;
    Stimulus.Timestamp = GetWorld()->GetTimeSeconds();
    Stimulus.StimulusTag = GeneratePerceptionTag(UpdateInfo.Target.Get(), UpdateInfo.Stimulus.Type);
    
    Stimulus.StimulusData.Add(TEXT("Age"), FString::SanitizeFloat(UpdateInfo.Stimulus.GetAge()));
    Stimulus.StimulusData.Add(TEXT("WasSuccessfullySensed"), UpdateInfo.Stimulus.WasSuccessfullySensed() ? TEXT("1") : TEXT("0"));
    
    return Stimulus;
}

EARPG_StimulusType UARPG_AIPerceptionComponent::GetStimulusTypeFromSenseID(const FAISenseID& SenseID) const
{
    // Compare with known sense IDs
    if (SenseID == UAISense::GetSenseID<UAISense_Sight>())
    {
        return EARPG_StimulusType::Visual;
    }
    else if (SenseID == UAISense::GetSenseID<UAISense_Hearing>())
    {
        return EARPG_StimulusType::Audio;
    }
    else if (SenseID == UAISense::GetSenseID<UAISense_Touch>())
    {
        return EARPG_StimulusType::Touch;
    }
    
    return EARPG_StimulusType::Visual;
}

TSubclassOf<UAISense> UARPG_AIPerceptionComponent::GetSenseClassFromSenseID(const FAISenseID& SenseID) const
{
    // Compare with known sense IDs
    if (SenseID == UAISense::GetSenseID<UAISense_Sight>())
    {
        return UAISense_Sight::StaticClass();
    }
    if (SenseID == UAISense::GetSenseID<UAISense_Hearing>())
    {
        return UAISense_Hearing::StaticClass();
    }
    if (SenseID == UAISense::GetSenseID<UAISense_Touch>())
    {
        return UAISense_Touch::StaticClass();
    }
    
    return UAISense_Sight::StaticClass();
}

float UARPG_AIPerceptionComponent::CalculateStimulusIntensity(const FActorPerceptionUpdateInfo& UpdateInfo) const
{
    float BaseIntensity = 0.5f;
    
    if (UpdateInfo.Stimulus.Strength > 0.0f)
    {
        BaseIntensity = UpdateInfo.Stimulus.Strength;
    }
    
    if (AActor* Owner = GetOwner())
    {
        float Distance = FVector::Dist(Owner->GetActorLocation(), UpdateInfo.Stimulus.StimulusLocation);
        float MaxRange = PerceptionConfig.SightRadius;
        
        // Determine max range based on sense type
        if (UpdateInfo.Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
        {
            MaxRange = PerceptionConfig.HearingRadius;
        }
        else if (UpdateInfo.Stimulus.Type == UAISense::GetSenseID<UAISense_Touch>())
        {
            MaxRange = PerceptionConfig.TouchRadius;
        }
        
        if (MaxRange > 0.0f)
        {
            float DistanceFactor = 1.0f - FMath::Clamp(Distance / MaxRange, 0.0f, 1.0f);
            BaseIntensity *= (0.3f + 0.7f * DistanceFactor);
        }
    }
    
    float Age = UpdateInfo.Stimulus.GetAge();
    float AgeFactor = FMath::Exp(-Age * 0.5f);
    BaseIntensity *= AgeFactor;
    
    if (UpdateInfo.Stimulus.WasSuccessfullySensed())
    {
        BaseIntensity *= 1.2f;
    }
    
    return FMath::Clamp(BaseIntensity, 0.0f, 1.0f);
}

FGameplayTag UARPG_AIPerceptionComponent::GeneratePerceptionTag(AActor* PerceivedActor, const FAISenseID& SenseID) const
{
    if (!IsValid(PerceivedActor))
    {
        return FGameplayTag::RequestGameplayTag(TEXT("AI.Perception.Unknown"));
    }
    
    FString BaseTagString = TEXT("AI.Perception.");
    
    // Determine sense type based on SenseID comparison
    if (SenseID == UAISense::GetSenseID<UAISense_Sight>())
    {
        BaseTagString += TEXT("Sight.");
    }
    else if (SenseID == UAISense::GetSenseID<UAISense_Hearing>())
    {
        BaseTagString += TEXT("Sound.");
    }
    else if (SenseID == UAISense::GetSenseID<UAISense_Touch>())
    {
        BaseTagString += TEXT("Touch.");
    }
    else
    {
        BaseTagString += TEXT("Generic.");
    }
    
    if (PerceivedActor->IsA<APawn>())
    {
        BaseTagString += TEXT("Character");
    }
    else
    {
        if (PerceivedActor->ActorHasTag(FName("Enemy")))
        {
            BaseTagString += TEXT("Enemy");
        }
        else if (PerceivedActor->ActorHasTag(FName("Friendly")))
        {
            BaseTagString += TEXT("Friendly");
        }
        else if (PerceivedActor->ActorHasTag(FName("Item")))
        {
            BaseTagString += TEXT("Item");
        }
        else
        {
            BaseTagString += TEXT("Generic");
        }
    }
    
    return FGameplayTag::RequestGameplayTag(FName(*BaseTagString));
}

void UARPG_AIPerceptionComponent::HandlePerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    if (!BrainComponent.IsValid())
    {
        return;
    }
    
    for (AActor* Actor : UpdatedActors)
    {
        if (!IsValid(Actor))
        {
            continue;
        }
        
        FActorPerceptionBlueprintInfo PerceptionInfo;
        if (GetActorsPerception(Actor, PerceptionInfo))
        {
            for (const FAIStimulus& AIStimulus : PerceptionInfo.LastSensedStimuli)
            {
                FActorPerceptionUpdateInfo UpdateInfo;
                UpdateInfo.Target = Actor;
                UpdateInfo.Stimulus = AIStimulus;
                
                if (BP_ShouldGenerateStimulus(Actor, GetSenseClassFromSenseID(AIStimulus.Type)))
                {
                    FARPG_AIStimulus Stimulus = ConvertPerceptionToStimulus(UpdateInfo);
                    FARPG_AIStimulus ModifiedStimulus = BP_ModifyGeneratedStimulus(Stimulus, Actor);
                    
                    BrainComponent->ProcessStimulus(ModifiedStimulus);
                    OnPerceptionStimulus.Broadcast(ModifiedStimulus, Actor);
                    
                    if (PerceptionConfig.bEnableDebugLogging)
                    {
                        UE_LOG(LogTemp, VeryVerbose, TEXT("Generated stimulus for %s: %s (%.2f)"), 
                               *Actor->GetName(), *ModifiedStimulus.StimulusTag.ToString(), ModifiedStimulus.Intensity);
                    }
                }
            }
        }
    }
}

void UARPG_AIPerceptionComponent::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!BrainComponent.IsValid() || !IsValid(Actor))
    {
        return;
    }
    
    FActorPerceptionUpdateInfo UpdateInfo;
    UpdateInfo.Target = Actor;
    UpdateInfo.Stimulus = Stimulus;
    
    if (BP_ShouldGenerateStimulus(Actor, GetSenseClassFromSenseID(Stimulus.Type)))
    {
        FARPG_AIStimulus ARPGStimulus = ConvertPerceptionToStimulus(UpdateInfo);
        FARPG_AIStimulus ModifiedStimulus = BP_ModifyGeneratedStimulus(ARPGStimulus, Actor);
        
        BrainComponent->ProcessStimulus(ModifiedStimulus);
        OnPerceptionStimulus.Broadcast(ModifiedStimulus, Actor);
        
        if (PerceptionConfig.bEnableDebugLogging)
        {
            UE_LOG(LogTemp, VeryVerbose, TEXT("Target perception updated for %s: %s (%.2f)"), 
                   *Actor->GetName(), *ModifiedStimulus.StimulusTag.ToString(), ModifiedStimulus.Intensity);
        }
    }
}
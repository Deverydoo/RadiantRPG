// Private/Characters/BaseCharacter.cpp
// Fixed character implementation with SAFE dynamic component creation

#include "Characters/BaseCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameplayTagsManager.h"

// Component includes
#include "Components/HealthComponent.h"
#include "Components/ManaComponent.h"
#include "Components/StaminaComponent.h"
#include "Components/SkillsComponent.h"
#include "Components/NeedsComponent.h"

ABaseCharacter::ABaseCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    // Optimize tick rate for NPCs - will be overridden by PlayerCharacter
    PrimaryActorTick.TickInterval = 0.1f; // 10Hz for NPCs

    // Set default capsule size
    GetCapsuleComponent()->SetCapsuleSize(42.0f, 96.0f);

    // Default character movement settings
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 700.0f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 300.0f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

    // Initialize core properties
    CharacterType = ECharacterType::NPC;
    CurrentState = ECharacterState::Idle;
    PreviousState = ECharacterState::Idle;
    bIsAlive = true;
    bIsInvulnerable = false;

    // Initialize cached stats
    CurrentHealth = 0.0f;
    MaxHealth = 0.0f;
    CurrentMana = 0.0f;
    MaxMana = 0.0f;
    CurrentStamina = 0.0f;
    MaxStamina = 0.0f;

    // Create mandatory core components IN CONSTRUCTOR - ALWAYS SAFE
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
    NeedsComponent = CreateDefaultSubobject<UNeedsComponent>(TEXT("NeedsComponent"));

    // For player characters, create all components in constructor for safety
    // Create core components for ALL characters
    StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
    ManaComponent = CreateDefaultSubobject<UManaComponent>(TEXT("ManaComponent"));

    // Skills component - only create for players
    if (CharacterType == ECharacterType::Player)
    {
        SkillsComponent = CreateDefaultSubobject<USkillsComponent>(TEXT("SkillsComponent"));
    }
    else
    {
        SkillsComponent = nullptr;
    }

    // Enable replication
    bReplicates = true;
    SetReplicatingMovement(true);
}

void ABaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    // SAFE: Initialize components that were created in constructor ONLY
    InitializeExistingComponents();

    // Load character data from DataTable if specified
    LoadCharacterData();

    // Apply character tags
    ApplyCharacterTags();

    // Bind component events for all existing components
    BindAllComponentEvents();

    UE_LOG(LogTemp, Log, TEXT("BaseCharacter '%s' initialized as %s"), 
           *GetName(), 
           *UEnum::GetValueAsString(CharacterType));
}

void ABaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clean up any dynamic components or timers
    Super::EndPlay(EndPlayReason);
}

void ABaseCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Minimal tick logic - most work should be event-driven
    // Only update critical cached values if they've changed significantly
    
    if (HealthComponent)
    {
        float NewHealth = HealthComponent->GetHealth();
        float NewMaxHealth = HealthComponent->GetMaxHealth();
        
        if (FMath::Abs(NewHealth - CurrentHealth) > 0.1f || 
            FMath::Abs(NewMaxHealth - MaxHealth) > 0.1f)
        {
            CurrentHealth = NewHealth;
            MaxHealth = NewMaxHealth;
        }
    }
}

void ABaseCharacter::InitializeExistingComponents()
{
    // Only initialize components that were created in the constructor
    // This is SAFE because components already exist
    
    if (HealthComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("Initialized HealthComponent for %s"), *GetName());
    }

    if (NeedsComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("Initialized NeedsComponent for %s"), *GetName());
    }

    if (StaminaComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("Initialized StaminaComponent for %s"), *GetName());
    }

    if (ManaComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("Initialized ManaComponent for %s"), *GetName());
    }

    if (SkillsComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("Initialized SkillsComponent for %s"), *GetName());
    }
}

void ABaseCharacter::LoadCharacterData()
{
    // TODO: Implement DataTable loading
    // This would load base stats, default tags, component configurations, etc.
    // from a CharacterDataTable based on CharacterDataRowName
    
    if (CharacterDataRowName.IsNone())
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Character data loading not yet implemented for row: %s"), 
           *CharacterDataRowName.ToString());
}

void ABaseCharacter::ApplyCharacterTags()
{
    // Apply actor tags for quick identification
    for (const FName& ActorTag : CharacterActorTags)
    {
        Tags.AddUnique(ActorTag);
    }

    // Apply gameplay tags to the character
    // These would be used by abilities, AI, and other systems
}

void ABaseCharacter::BindAllComponentEvents()
{
    // Bind events for all existing components
    if (HealthComponent)
    {
        HealthComponent->OnHealthChanged.AddDynamic(this, &ABaseCharacter::OnHealthComponentChanged);
        HealthComponent->OnDeath.AddDynamic(this, &ABaseCharacter::OnCharacterDeathInternal);
    }

    if (ManaComponent)
    {
        ManaComponent->OnManaChanged.AddDynamic(this, &ABaseCharacter::OnManaComponentChanged);
    }

    if (StaminaComponent)
    {
        StaminaComponent->OnStaminaChanged.AddDynamic(this, &ABaseCharacter::OnStaminaComponentChanged);
    }

    UE_LOG(LogTemp, Log, TEXT("Component events bound for %s"), *GetName());
}

bool ABaseCharacter::HasCharacterTag(const FGameplayTag& Tag) const
{
    return CharacterTags.HasTag(Tag);
}

bool ABaseCharacter::HasAnyCharacterTags(const FGameplayTagContainer& InTags) const
{
    return CharacterTags.HasAny(InTags);
}

void ABaseCharacter::SetCharacterState(ECharacterState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }

    PreviousState = CurrentState;
    CurrentState = NewState;

    // Broadcast state change
    OnCharacterStateChanged.Broadcast(CurrentState);
    OnCharacterStateChangedBP(PreviousState, CurrentState);

    UE_LOG(LogTemp, Log, TEXT("Character %s state changed from %s to %s"),
           *GetName(),
           *UEnum::GetValueAsString(PreviousState),
           *UEnum::GetValueAsString(CurrentState));
}

void ABaseCharacter::SetFactionTag(const FGameplayTag& NewFactionTag)
{
    if (FactionTag != NewFactionTag)
    {
        FGameplayTag OldFaction = FactionTag;
        FactionTag = NewFactionTag;

        UE_LOG(LogTemp, Log, TEXT("Character %s faction changed from %s to %s"),
               *GetName(),
               *OldFaction.ToString(),
               *FactionTag.ToString());
    }
}

void ABaseCharacter::Die()
{
    if (!bIsAlive)
    {
        return;
    }

    bIsAlive = false;
    SetCharacterState(ECharacterState::Dead);

    // Disable collision and movement
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetCharacterMovement()->DisableMovement();

    // Broadcast death event
    OnCharacterDeath.Broadcast(this);
    OnCharacterDeathBP();

    UE_LOG(LogTemp, Warning, TEXT("Character %s has died"), *GetName());
}

void ABaseCharacter::Revive(float HealthPercentage)
{
    if (bIsAlive)
    {
        return;
    }

    bIsAlive = true;
    SetCharacterState(ECharacterState::Idle);

    // Re-enable collision and movement
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    // Restore health
    if (HealthComponent)
    {
        float NewHealth = HealthComponent->GetMaxHealth() * FMath::Clamp(HealthPercentage, 0.0f, 1.0f);
        HealthComponent->SetHealth(NewHealth);
    }

    OnCharacterReviveBP(HealthPercentage);

    UE_LOG(LogTemp, Log, TEXT("Character %s has been revived with %.1f%% health"), 
           *GetName(), HealthPercentage * 100.0f);
}

// SAFE RUNTIME COMPONENT MANAGEMENT - Only for post-BeginPlay usage
UActorComponent* ABaseCharacter::AddCharacterComponent(TSubclassOf<UActorComponent> ComponentClass, const FString& ComponentName)
{
    if (!ComponentClass || HasComponent(ComponentClass))
    {
        return nullptr;
    }

    // Only allow runtime component addition AFTER BeginPlay has completed
    if (!HasActorBegunPlay())
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot add components before BeginPlay completes"));
        return nullptr;
    }

    UActorComponent* NewComponent = NewObject<UActorComponent>(this, ComponentClass, *ComponentName);
    if (NewComponent)
    {
        AddOwnedComponent(NewComponent);
        AddedComponentTypes.Add(ComponentClass);
        NewComponent->RegisterComponent();
        
        // Since we're post-BeginPlay, manually call BeginPlay on the component
        NewComponent->BeginPlay();
        
        BindComponentEvents(NewComponent);

        UE_LOG(LogTemp, Log, TEXT("Dynamically added component %s to character %s"), 
               *ComponentClass->GetName(), *GetName());
    }

    return NewComponent;
}

bool ABaseCharacter::RemoveCharacterComponent(UActorComponent* Component)
{
    if (!Component || Component->GetOwner() != this)
    {
        return false;
    }

    TSubclassOf<UActorComponent> ComponentClass = Component->GetClass();
    
    Component->DestroyComponent();
    AddedComponentTypes.Remove(ComponentClass);

    UE_LOG(LogTemp, Log, TEXT("Removed component %s from character %s"), 
           *ComponentClass->GetName(), *GetName());

    return true;
}

bool ABaseCharacter::HasComponent(TSubclassOf<UActorComponent> ComponentClass) const
{
    return GetComponentByClass(ComponentClass) != nullptr;
}

void ABaseCharacter::BindComponentEvents(UActorComponent* Component)
{
    // Bind events specific to each component type
    if (UHealthComponent* Health = Cast<UHealthComponent>(Component))
    {
        Health->OnHealthChanged.AddDynamic(this, &ABaseCharacter::OnHealthComponentChanged);
        Health->OnDeath.AddDynamic(this, &ABaseCharacter::OnCharacterDeathInternal);
    }
    else if (UManaComponent* Mana = Cast<UManaComponent>(Component))
    {
        Mana->OnManaChanged.AddDynamic(this, &ABaseCharacter::OnManaComponentChanged);
    }
    else if (UStaminaComponent* Stamina = Cast<UStaminaComponent>(Component))
    {
        Stamina->OnStaminaChanged.AddDynamic(this, &ABaseCharacter::OnStaminaComponentChanged);
    }
}

void ABaseCharacter::OnHealthComponentChanged(float NewHealth, float NewMaxHealth)
{
    CurrentHealth = NewHealth;
    MaxHealth = NewMaxHealth;
    
    float HealthPercent = MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
    float DamageAmount = 0.0f; // Could track this if needed
    
    OnCharacterHealthChanged.Broadcast(HealthPercent, DamageAmount);
}

void ABaseCharacter::OnManaComponentChanged(float NewMana, float NewMaxMana)
{
    CurrentMana = NewMana;
    MaxMana = NewMaxMana;
}

void ABaseCharacter::OnStaminaComponentChanged(float NewStamina, float NewMaxStamina)
{
    CurrentStamina = NewStamina;
    MaxStamina = NewMaxStamina;
}

void ABaseCharacter::OnCharacterDeathInternal(ABaseCharacter* DeadCharacter)
{
    // Called by health component when health reaches 0
    Die();
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABaseCharacter, CurrentState);
    DOREPLIFETIME(ABaseCharacter, bIsAlive);
    DOREPLIFETIME(ABaseCharacter, FactionTag);
}

void ABaseCharacter::OnRep_CharacterState()
{
    // Handle state replication on clients
    OnCharacterStateChanged.Broadcast(CurrentState);
    OnCharacterStateChangedBP(PreviousState, CurrentState);
}
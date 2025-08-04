// Public/Characters/BaseCharacter.h
// Fixed character header with SAFE component creation architecture

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "BaseCharacter.generated.h"

// Forward declarations
class UHealthComponent;
class UManaComponent;
class UStaminaComponent;
class USkillsComponent;
class UNeedsComponent;
class UBrainComponent;
class UPerceptionComponent;

UENUM(BlueprintType)
enum class ECharacterType : uint8
{
    Player UMETA(DisplayName = "Player"),
    NPC UMETA(DisplayName = "NPC"),
    Creature UMETA(DisplayName = "Creature"),
    Construct UMETA(DisplayName = "Construct")
};

UENUM(BlueprintType)
enum class ECharacterState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Moving UMETA(DisplayName = "Moving"),
    InCombat UMETA(DisplayName = "In Combat"),
    Dead UMETA(DisplayName = "Dead"),
    Unconscious UMETA(DisplayName = "Unconscious"),
    Stunned UMETA(DisplayName = "Stunned")
};

// Core character events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChanged, ECharacterState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterDeath, ABaseCharacter*, DeadCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterHealthChanged, float, NewHealthPercent, float, DamageAmount);

/**
 * BaseCharacter - Core character class for RadiantRPG
 * 
 * SAFETY-FIRST Design Philosophy:
 * - All critical components created in constructor for safety
 * - No dynamic component creation during BeginPlay
 * - Event-driven communication between systems
 * - Data-driven configuration via DataTables and GameplayTags
 * - Clear separation of player-specific and NPC-specific logic
 * - Performance optimized for large numbers of NPCs
 * 
 * Components are created in constructor based on character type:
 * - All characters: Health, basic needs
 * - Player characters: All components (Stamina, Mana, Skills)
 * - NPCs: Only components they actually need
 */
UCLASS(Abstract, Blueprintable)
class RADIANTRPG_API ABaseCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ABaseCharacter();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    // === CHARACTER CONFIGURATION ===
    
    /** What type of character this is - affects which components are created */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Config")
    ECharacterType CharacterType;

    /** Gameplay tags that define this character's capabilities and traits */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Config")
    FGameplayTagContainer CharacterTags;

    /** Actor tags for quick identification by AI and systems */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Config")
    TArray<FName> CharacterActorTags;

    /** DataTable row that defines this character's base stats and configuration */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Config")
    FName CharacterDataRowName;

    // === CORE COMPONENTS (Always Present) ===
    
    /** Health management - always present */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core Components")
    class UHealthComponent* HealthComponent;

    /** Basic needs (hunger, sleep, etc.) - always present */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core Components")
    class UNeedsComponent* NeedsComponent;

    // === OPTIONAL COMPONENTS (Created based on character type) ===
    
    /** Mana/Magic system - created for magic users */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Optional Components")
    class UManaComponent* ManaComponent;

    /** Physical stamina - created for physical beings */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Optional Components")
    class UStaminaComponent* StaminaComponent;

    /** Skill progression system - created for learning entities */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Optional Components")
    class USkillsComponent* SkillsComponent;

    // === AI COMPONENTS (NPCs only) ===
    
    /** AI decision making brain - NPCs only */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    class UBrainComponent* BrainComponent;

    /** Sensory perception system - NPCs only */  
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    //class UPerceptionComponent* PerceptionComponent;

    /** Memory system for NPCs - stores experiences and knowledge */
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI Components")
    //class UMemoryComponent* MemoryComponent;

    // === CHARACTER STATE ===
    
    /** Current character state */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CharacterState, Category = "Character State")
    ECharacterState CurrentState;

    /** Whether this character is currently alive */
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Character State")
    bool bIsAlive;

    /** Whether this character can be damaged */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character State")
    bool bIsInvulnerable;

    /** Current faction this character belongs to */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Character State")
    FGameplayTag FactionTag;

    // === CACHED STATS (For performance) ===
    
    UPROPERTY(BlueprintReadOnly, Category = "Cached Stats")
    float CurrentHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Cached Stats")
    float MaxHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Cached Stats")
    float CurrentMana;

    UPROPERTY(BlueprintReadOnly, Category = "Cached Stats")
    float MaxMana;

    UPROPERTY(BlueprintReadOnly, Category = "Cached Stats")
    float CurrentStamina;

    UPROPERTY(BlueprintReadOnly, Category = "Cached Stats")
    float MaxStamina;

public:
    // === EVENTS ===
    
    UPROPERTY(BlueprintAssignable, Category = "Character Events")
    FOnCharacterStateChanged OnCharacterStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Character Events")
    FOnCharacterDeath OnCharacterDeath;

    UPROPERTY(BlueprintAssignable, Category = "Character Events")
    FOnCharacterHealthChanged OnCharacterHealthChanged;

    // === PUBLIC INTERFACE ===
    
    /** Get character type */
    UFUNCTION(BlueprintPure, Category = "Character")
    ECharacterType GetCharacterType() const { return CharacterType; }

    /** Check if character has a specific gameplay tag */
    UFUNCTION(BlueprintPure, Category = "Character")
    bool HasCharacterTag(const FGameplayTag& Tag) const;

    /** Check if character has any tags from a container */
    UFUNCTION(BlueprintPure, Category = "Character")
    bool HasAnyCharacterTags(const FGameplayTagContainer& InTags) const;

    /** Get current character state */
    UFUNCTION(BlueprintPure, Category = "Character")
    ECharacterState GetCharacterState() const { return CurrentState; }

    /** Set character state */
    UFUNCTION(BlueprintCallable, Category = "Character")
    void SetCharacterState(ECharacterState NewState);

    /** Check if character is alive */
    UFUNCTION(BlueprintPure, Category = "Character")
    bool IsAlive() const { return bIsAlive; }

    /** Get faction tag */
    UFUNCTION(BlueprintPure, Category = "Character")
    FGameplayTag GetFactionTag() const { return FactionTag; }

    /** Set faction tag */
    UFUNCTION(BlueprintCallable, Category = "Character")
    void SetFactionTag(const FGameplayTag& NewFactionTag);

    // === COMPONENT ACCESSORS ===
    
    UFUNCTION(BlueprintPure, Category = "Components")
    UHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    UManaComponent* GetManaComponent() const { return ManaComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    UStaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    USkillsComponent* GetSkillsComponent() const { return SkillsComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    UNeedsComponent* GetNeedsComponent() const { return NeedsComponent; }

    UFUNCTION(BlueprintPure, Category = "Components")
    UBrainComponent* GetBrainComponent() const { return BrainComponent; }

    //UFUNCTION(BlueprintPure, Category = "Components")
    //UPerceptionComponent* GetPerceptionComponent() const { return PerceptionComponent; }

    //UFUNCTION(BlueprintPure, Category = "Components")
    //UMemoryComponent* GetMemoryComponent() const { return MemoryComponent; }

    // === DEATH AND REVIVAL ===
    
    /** Kill this character */
    UFUNCTION(BlueprintCallable, Category = "Character")
    virtual void Die();

    /** Revive this character */
    UFUNCTION(BlueprintCallable, Category = "Character")
    virtual void Revive(float HealthPercentage = 1.0f);

    // === SAFE RUNTIME COMPONENT MANAGEMENT ===
    
    /** Add a component to this character at runtime (ONLY after BeginPlay) */
    UFUNCTION(BlueprintCallable, Category = "Character")
    UActorComponent* AddCharacterComponent(TSubclassOf<UActorComponent> ComponentClass, const FString& ComponentName);

    /** Remove a component from this character */
    UFUNCTION(BlueprintCallable, Category = "Character")
    bool RemoveCharacterComponent(UActorComponent* Component);

    /** Check if character has a specific component type */
    UFUNCTION(BlueprintPure, Category = "Character")
    bool HasComponent(TSubclassOf<UActorComponent> ComponentClass) const;

protected:
    // === INITIALIZATION ===
    
    /** Initialize components that were created in constructor */
    virtual void InitializeExistingComponents();

    /** Load character data from DataTable if specified */
    virtual void LoadCharacterData();

    /** Apply character tags to actor */
    virtual void ApplyCharacterTags();

    /** Bind events for all existing components */
    virtual void BindAllComponentEvents();

    // === COMPONENT EVENT HANDLERS ===
    
    UFUNCTION()
    virtual void OnHealthComponentChanged(float NewHealth, float NewMaxHealth);

    UFUNCTION()
    virtual void OnManaComponentChanged(float NewMana, float NewMaxMana);

    UFUNCTION()
    virtual void OnStaminaComponentChanged(float NewStamina, float NewMaxStamina);

    UFUNCTION()
    virtual void OnCharacterDeathInternal(ABaseCharacter* DeadCharacter);

    // === REPLICATION ===
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    virtual void OnRep_CharacterState();

    // === BLUEPRINT EVENTS ===
    
    /** Called when character state changes */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character Events")
    void OnCharacterStateChangedBP(ECharacterState OldState, ECharacterState NewState);

    /** Called when character dies */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character Events")
    void OnCharacterDeathBP();

    /** Called when character is revived */
    UFUNCTION(BlueprintImplementableEvent, Category = "Character Events")
    void OnCharacterReviveBP(float HealthPercentage);

    // === UTILITY ===
    
    /** Bind events from a component */
    virtual void BindComponentEvents(UActorComponent* Component);

private:
    /** Track which optional components have been added */
    UPROPERTY()
    TSet<TSubclassOf<UActorComponent>> AddedComponentTypes;

    /** Previous state for change detection */
    ECharacterState PreviousState;
};
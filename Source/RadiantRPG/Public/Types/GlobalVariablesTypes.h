// Source/RadiantRPG/Public/Types/GlobalVariablesTypes.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GlobalVariablesTypes.generated.h"

// Float Variables
USTRUCT(BlueprintType)
struct RADIANTRPG_API FGlobalVariableEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value;

    FGlobalVariableEntry()
        : Key(TEXT(""))
        , Value(0.0f)
    {
    }

    FGlobalVariableEntry(const FString& InKey, float InValue)
        : Key(InKey)
        , Value(InValue)
    {
    }

    bool operator==(const FGlobalVariableEntry& Other) const
    {
        return Key == Other.Key && Value == Other.Value;
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << Key;
        Ar << Value;
        bOutSuccess = true;
        return true;
    }

    // Add regular archive serialization operator
    friend FArchive& operator<<(FArchive& Ar, FGlobalVariableEntry& Entry)
    {
        Ar << Entry.Key;
        Ar << Entry.Value;
        return Ar;
    }
};

template<>
struct TStructOpsTypeTraits<FGlobalVariableEntry> : public TStructOpsTypeTraitsBase2<FGlobalVariableEntry>
{
    enum
    {
        WithNetSerializer = true,
        WithIdenticalViaEquality = true,
    };
};

// String Variables
USTRUCT(BlueprintType)
struct RADIANTRPG_API FGlobalStringEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Value;

    FGlobalStringEntry()
        : Key(TEXT(""))
        , Value(TEXT(""))
    {
    }

    FGlobalStringEntry(const FString& InKey, const FString& InValue)
        : Key(InKey)
        , Value(InValue)
    {
    }

    bool operator==(const FGlobalStringEntry& Other) const
    {
        return Key == Other.Key && Value == Other.Value;
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << Key;
        Ar << Value;
        bOutSuccess = true;
        return true;
    }

    // Add regular archive serialization operator
    friend FArchive& operator<<(FArchive& Ar, FGlobalStringEntry& Entry)
    {
        Ar << Entry.Key;
        Ar << Entry.Value;
        return Ar;
    }
};

template<>
struct TStructOpsTypeTraits<FGlobalStringEntry> : public TStructOpsTypeTraitsBase2<FGlobalStringEntry>
{
    enum
    {
        WithNetSerializer = true,
        WithIdenticalViaEquality = true,
    };
};

USTRUCT(BlueprintType)
struct RADIANTRPG_API FGlobalVariablesContainer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGlobalVariableEntry> Variables;

    FGlobalVariablesContainer()
    {
    }

    // Helper functions to mimic TMap behavior
    void SetVariable(const FString& Key, float Value)
    {
        for (FGlobalVariableEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                Entry.Value = Value;
                return;
            }
        }
        Variables.Add(FGlobalVariableEntry(Key, Value));
    }

    float GetVariable(const FString& Key, float DefaultValue = 0.0f) const
    {
        for (const FGlobalVariableEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                return Entry.Value;
            }
        }
        return DefaultValue;
    }

    bool HasVariable(const FString& Key) const
    {
        for (const FGlobalVariableEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                return true;
            }
        }
        return false;
    }

    bool RemoveVariable(const FString& Key)
    {
        for (int32 i = 0; i < Variables.Num(); ++i)
        {
            if (Variables[i].Key == Key)
            {
                Variables.RemoveAt(i);
                return true;
            }
        }
        return false;
    }

    void GetKeys(TArray<FString>& OutKeys) const
    {
        OutKeys.Reset();
        for (const FGlobalVariableEntry& Entry : Variables)
        {
            OutKeys.Add(Entry.Key);
        }
    }

    int32 Num() const
    {
        return Variables.Num();
    }

    void Empty()
    {
        Variables.Empty();
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << Variables;
        bOutSuccess = true;
        return true;
    }
};

USTRUCT(BlueprintType)
struct RADIANTRPG_API FGlobalStringsContainer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGlobalStringEntry> Variables;

    FGlobalStringsContainer()
    {
    }

    // Helper functions to mimic TMap behavior
    void SetVariable(const FString& Key, const FString& Value)
    {
        for (FGlobalStringEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                Entry.Value = Value;
                return;
            }
        }
        Variables.Add(FGlobalStringEntry(Key, Value));
    }

    FString GetVariable(const FString& Key, const FString& DefaultValue = TEXT("")) const
    {
        for (const FGlobalStringEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                return Entry.Value;
            }
        }
        return DefaultValue;
    }

    bool HasVariable(const FString& Key) const
    {
        for (const FGlobalStringEntry& Entry : Variables)
        {
            if (Entry.Key == Key)
            {
                return true;
            }
        }
        return false;
    }

    bool RemoveVariable(const FString& Key)
    {
        for (int32 i = 0; i < Variables.Num(); ++i)
        {
            if (Variables[i].Key == Key)
            {
                Variables.RemoveAt(i);
                return true;
            }
        }
        return false;
    }

    void GetKeys(TArray<FString>& OutKeys) const
    {
        OutKeys.Reset();
        for (const FGlobalStringEntry& Entry : Variables)
        {
            OutKeys.Add(Entry.Key);
        }
    }

    int32 Num() const
    {
        return Variables.Num();
    }

    void Empty()
    {
        Variables.Empty();
    }

    bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
    {
        Ar << Variables;
        bOutSuccess = true;
        return true;
    }
};
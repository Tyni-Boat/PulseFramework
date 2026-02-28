// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameSaveProvider.h"
#include "LocalGameSaveProvider.generated.h"


UCLASS(NotBlueprintType, NotBlueprintable)
class PULSEGAMEFRAMEWORK_API ULocalSaveMeta : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = Basic)
	FSaveMetaData SavedMetaData;
};

/**
 * Save game Provider To Save Game In Local Disk
 */
UCLASS(BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API ULocalGameSaveProvider : public UGameSaveProvider
{
	GENERATED_BODY()

protected:
	int32 _saveSlotCount = 1;
	int32 _bufferIndexesSize = 1;
	FVector2D _saved_GameX_MetaY;
	TQueue<FString> _metaLoadSlotsQueue;
	TArray<FSaveMetaData> _loadedMetaDataList;

	UFUNCTION()
	void OnSavedGame_Internal(const FString& SlotName, const int32 UserIndex, bool bSuccess);

	UFUNCTION()
	void OnLoadedGame_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);

	UFUNCTION()
	void OnSavedMeta_Internal(const FString& SlotName, const int32 UserIndex, bool bSuccess);

	UFUNCTION()
	void OnLoadedMeta_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedMetaData);

public:
	ULocalGameSaveProvider();
	virtual void Initialization_Implementation(UPulseSaveManager* SaveManager, UCoreProjectSetting* ProjectSettings) override;
	virtual void Deinitialization_Implementation(UPulseSaveManager* SaveManager) override;
	virtual void BeginSave_Implementation(const FUserProfile& User, const FSaveMetaData& SaveMeta, UPulseSaveData* SaveData) override;
	virtual void BeginLoadMeta_Implementation(const FUserProfile& Userprofile) override;
	virtual void BeginLoadGame_Implementation(const FUserProfile& Userprofile, const FSaveMetaData& Meta) override;
	virtual void BeginDeleteGame_Implementation(const FUserProfile& User, const FSaveMetaData& Meta) override;
	virtual bool IsLastSavedMeta_Implementation(const FSaveMetaData& Meta) const override;
	virtual int32 GetBufferSlot_Implementation(const int32 SlotIndex, bool bIsAutoSaveSlot = false) override;
};

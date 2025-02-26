// Fill out your copyright notice in the Description page of Project Settings.

#include "Inputs/InputConfig.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Move, "Input.Movement")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_TurnMouse, "Input.Turn.Mouse")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_TurnGamepad, "Input.Turn.Gamepad")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Jump, "Input.Jump")

const UInputAction* UInputConfig::FindInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FTaggedInputAction& TaggedInputAction : TaggedInputActions)
	{
		if (TaggedInputAction.InputAction && TaggedInputAction.InputTag == InputTag)
		{
			return TaggedInputAction.InputAction;
		}
	}

	return nullptr;
}

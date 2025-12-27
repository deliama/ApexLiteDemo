// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ApexCharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UApexMovementComponent::UApexMovementComponent()
{
	NavAgentProps.bCanCrouch = true;
}



// -------------------------------------------------------------------
// 网络预测部分 (Network Prediction Boilerplate)
// -------------------------------------------------------------------

// 1. 将自定义标记打包进 Flags
void UApexMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);
	// FLAG_Custom_0 是引擎预留给我们的自定义标记位
	bWantsToSlide = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

// 2. 客户端：创建并分配 Move
FNetworkPredictionData_Client* UApexMovementComponent::GetPredictionData_Client() const
{
	if (!ClientPredictionData)
	{
		// 这是一个 const 函数，但我们需要初始化 mutable 成员，所以用 const_cast
		UApexMovementComponent* MutableThis = const_cast<UApexMovementComponent*>(this);
		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Apex(*this);
	}
	return ClientPredictionData;
}

// 3. 客户端数据结构实现
UApexMovementComponent::FNetworkPredictionData_Client_Apex::FNetworkPredictionData_Client_Apex(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UApexMovementComponent::FNetworkPredictionData_Client_Apex::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_Apex());
}

// 4. SavedMove 实现：保存状态以便发生回滚时重演
void UApexMovementComponent::FSavedMove_Apex::Clear()
{
	Super::Clear();
	bSavedWantsToSlide = 0;
}

uint8 UApexMovementComponent::FSavedMove_Apex::GetCompressedFlags() const
{
	uint8 Result = Super::GetCompressedFlags();
	if (bSavedWantsToSlide)
	{
		Result |= FLAG_Custom_0;
	}
	return Result;
}

bool UApexMovementComponent::FSavedMove_Apex::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	if (bSavedWantsToSlide != ((FSavedMove_Apex*)&NewMove)->bSavedWantsToSlide)
	{
		return false;
	}
	return Super::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UApexMovementComponent::FSavedMove_Apex::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData)
{
	Super::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);
	UApexMovementComponent* CharacterMovement = Cast<UApexMovementComponent>(C->GetCharacterMovement());
	if (CharacterMovement)
	{
		bSavedWantsToSlide = CharacterMovement->bWantsToSlide;
	}
}

void UApexMovementComponent::FSavedMove_Apex::PrepMoveFor(ACharacter* C)
{
	Super::PrepMoveFor(C);
	UApexMovementComponent* CharacterMovement = Cast<UApexMovementComponent>(C->GetCharacterMovement());
	if (CharacterMovement)
	{
		CharacterMovement->bWantsToSlide = bSavedWantsToSlide;
	}
}

// -------------------------------------------------------------------
// 移动逻辑部分 (Movement Logic)
// -------------------------------------------------------------------

void UApexMovementComponent::SprintPressed()
{
}

void UApexMovementComponent::SprintReleased()
{
}

void UApexMovementComponent::CrouchPressed()
{
	bWantsToSlide = true;
	// 如果已经在地面上且速度足够，这里会在下一帧 Update 时触发 EnterSlide
}

void UApexMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (!CharacterOwner) return;

	// // 状态机检查：如果我们想滑铲，且符合条件，进入 Custom 模式
	// if (bWantsToSlide && CanSlide())
	// {
	// 	EnterSlide();
	// }
	//
	// // 如果在滑铲中但速度太慢，或者不再想滑铲，退出
	// if (IsCustomMovementMode(CMOVE_Slide) && (!bWantsToSlide || Velocity.Size() < MinSlideSpeed))
	// {
	// 	ExitSlide();
	// }

	if(bWantsToSlide)
	{
		if(IsSliding())
		{
			if(Velocity.Size() < 100.0f)
			{
				ExitSlide();
			}
		}else if(CanSlide())
		{
			EnterSlide();
		}else if(!IsCrouching() && !IsFalling())
		{
			CharacterOwner->Crouch();
		}
	}else
	{
		if(IsSliding())
		{
			ExitSlide();
		}else if(IsCrouching())
		{
			CharacterOwner->UnCrouch();
		}
	}
}

bool UApexMovementComponent::CanSlide() const
{
	// 必须在地面上，且不是已经在滑铲，且速度达标
	return IsWalking() && !IsSliding() && Velocity.Size() >= MinSlideSpeed;
}

void UApexMovementComponent::EnterSlide()
{
	bWantsToSlide = true;
	SetMovementMode(MOVE_Custom, CMOVE_Slide);
	
	// 核心体验：开始时给予一个瞬间的冲量
	FVector SlideDirection = Velocity.GetSafeNormal();
	Velocity += SlideDirection * SlideImpulse;
	
	// 缩小碰撞体 (Crouch)
	CharacterOwner->Crouch(); 
}

void UApexMovementComponent::ExitSlide()
{
	bWantsToSlide = false;
	
	// 恢复为普通行走
	SetMovementMode(MOVE_Walking);
	CharacterOwner->UnCrouch();
}

bool UApexMovementComponent::IsSliding() const
{
	return IsCustomMovementMode(CMOVE_Slide);
}

// -------------------------------------------------------------------
// 物理模拟部分 (Physics Implementation)
// -------------------------------------------------------------------

void UApexMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	if (CustomMovementMode == CMOVE_Slide)
	{
		PhysSlide(deltaTime, Iterations);
	}
}

void UApexMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME) return;

	// 1. 获取并恢复滑铲前的物理状态
	RestorePreAdditiveRootMotionVelocity();

	// 2. 应用重力 (滑铲时可能希望重力影响更大，特别是下坡)
	Velocity.Z += GetGravityZ() * SlideGravityScale * deltaTime;

	// 3. 计算摩擦力和制动
	// 在 Apex 中，滑铲的摩擦力极低，但仍受地面法线影响
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, SlideFriction, true, GetMaxBrakingDeceleration());
	}
	
	// 4. 执行移动
	FHitResult Hit(1.f);
	SafeMoveUpdatedComponent(Velocity * deltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);

	// 5. 碰撞处理
	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, deltaTime, Velocity);
		SlideAlongSurface(Velocity, 1.f - Hit.Time, Hit.Normal, Hit, true);
	}

	// 6. 检查是否还在地面上 (如果滑出悬崖，转为 Falling)
	FFindFloorResult FloorResult;
	FindFloor(UpdatedComponent->GetComponentLocation(), FloorResult, false);
	if (!FloorResult.IsWalkableFloor())
	{
		ExitSlide(); // 或者 SetMovementMode(MOVE_Falling)
	}
}

// 实现辅助函数
bool UApexMovementComponent::IsCustomMovementMode(uint8 TestCustomMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == TestCustomMode;
}


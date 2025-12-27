// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ApexCharacterMovementComponent.generated.h"

/**
 * 自定义移动模式枚举
 * 扩展引擎默认的 EMovementMode
 */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_MAX			UMETA(Hidden),
};

UCLASS()
class APEXLEGENDS_API UApexMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	// 用于网络预测的 SavedMove 类
	class FSavedMove_Apex : public FSavedMove_Character
	{
	public:
		typedef FSavedMove_Character Super;

		// 这一帧是否请求了滑铲
		uint8 bSavedWantsToSlide : 1;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	// 客户端预测数据容器
	class FNetworkPredictionData_Client_Apex : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_Apex(const UCharacterMovementComponent& ClientMovement);
		typedef FNetworkPredictionData_Client_Character Super;
		virtual FSavedMovePtr AllocateNewMove() override;
	};

public:
	UApexMovementComponent();

	/* ---------------------------------------------------
	 * 核心接口
	 * --------------------------------------------------- */
	UFUNCTION(BlueprintCallable, Category = "Apex Movement")
	void SprintPressed();
	
	UFUNCTION(BlueprintCallable, Category = "Apex Movement")
	void SprintReleased();

	UFUNCTION(BlueprintCallable, Category = "Apex Movement")
	void CrouchPressed(); // 替代默认 Crouch，用于判断是否进入滑铲

	UFUNCTION(BlueprintPure, Category = "Apex Movement")
	bool IsSliding() const;

	// 检查当前是否处于特定的自定义移动模式
	UFUNCTION(BlueprintPure, Category = "Movement")
	bool IsCustomMovementMode(uint8 TestCustomMode) const;

protected:
	/* ---------------------------------------------------
	 * 引擎重写函数 (Override)
	 * --------------------------------------------------- */
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	// 处理具体的滑铲物理
	void PhysSlide(float deltaTime, int32 Iterations);
	
	// 进入和退出滑铲状态
	void EnterSlide();
	void ExitSlide();
	
	// 辅助检查
	bool CanSlide() const;

public:
	/* ---------------------------------------------------
	 * 配置参数 (Design Config)
	 * --------------------------------------------------- */
	// 滑铲提供的初始冲量
	UPROPERTY(EditDefaultsOnly, Category = "Apex Movement|Slide")
	float SlideImpulse = 500.f;

	// 滑铲时的最大速度
	UPROPERTY(EditDefaultsOnly, Category = "Apex Movement|Slide")
	float MaxSlideSpeed = 1200.f;

	// 滑铲时的地面摩擦力 (越小越滑)
	UPROPERTY(EditDefaultsOnly, Category = "Apex Movement|Slide")
	float SlideFriction = 0.1f;

	// 进入滑铲所需的最小速度
	UPROPERTY(EditDefaultsOnly, Category = "Apex Movement|Slide")
	float MinSlideSpeed = 350.f;

	// 滑铲时的重力缩放（如下坡加速）
	UPROPERTY(EditDefaultsOnly, Category = "Apex Movement|Slide")
	float SlideGravityScale = 2.0f;

private:
	// 标记：玩家是否有滑铲意图
	bool bWantsToSlide = false;
};
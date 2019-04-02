// Copyright (c) 2019, Team Sosweet. All rights reserved.

#include "SaucewichAnimInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

void USaucewichAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	APawn* Owner = TryGetPawnOwner();
	if (Owner)
	{
		FVector Velocity{ Owner->GetVelocity() };
		FRotator OwnerRot = Owner->GetActorRotation();
		MoveDirection = FRotator::NormalizeAxis(Velocity.Rotation().Yaw - OwnerRot.Yaw);
		MoveSpeed = Velocity.Size2D();

		FRotator NewAimOffset = Owner->GetBaseAimRotation() - OwnerRot;
		NewAimOffset.Normalize();
		AimOffset = NewAimOffset;

		float TargetHeadYaw = 0.f;
		if (FMath::Abs(AimOffset.Yaw) > 90.f)
		{
			TargetHeadYaw = FMath::Clamp(AimOffset.Yaw - 90.f * FMath::Sign(AimOffset.Yaw), MinHeadYaw, MaxHeadYaw);
			AimOffset.Yaw = FMath::Clamp(AimOffset.Yaw, -90.f, 90.f);
		}
		HeadYaw = FMath::FInterpTo(HeadYaw, TargetHeadYaw, DeltaTime, 10.f);

		bFalling = Owner->GetMovementComponent()->IsFalling();
	}
}

// Copyright (c) 2019, Team Sosweet. All rights reserved.

#include "SaucewichAnimInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

void USaucewichAnimInstance::NativeUpdateAnimation(const float DeltaTime)
{
	APawn* const Owner = TryGetPawnOwner();
	if (Owner)
	{
		const FVector Velocity{ Owner->GetVelocity() };
		const FRotator OwnerRot = Owner->GetActorRotation();

		MoveDirection = FRotator::NormalizeAxis(Velocity.Rotation().Yaw - OwnerRot.Yaw);
		MoveSpeed = Velocity.Size2D();

		AimOffset = Owner->GetBaseAimRotation() - OwnerRot;
		AimOffset.Normalize();

		HeadYaw = 0.f;
		if (FMath::Abs(AimOffset.Yaw) > 90.f)
		{
			HeadYaw = FMath::Clamp(AimOffset.Yaw - 90.f * FMath::Sign(AimOffset.Yaw), MinHeadYaw, MaxHeadYaw);
			AimOffset.Yaw = FMath::Clamp(AimOffset.Yaw, -90.f, 90.f);
		}

		bFalling = Owner->GetMovementComponent()->IsFalling();
	}
}

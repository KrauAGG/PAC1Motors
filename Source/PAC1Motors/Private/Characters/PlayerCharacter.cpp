// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/PlayerCharacter.h"

#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/AnimMontage.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
    
    //He suprimido a proposito los mensajes debug para mantener la pantalla y el log limpios
    PrimaryActorTick.bCanEverTick = true;

    /*
    // Mensaje en pantalla
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Hola des de C++"));
    }

    // Mensaje en el log de Unreal
    UE_LOG(LogTemp, Warning, TEXT("Hola des del log d'Unreal"));
    */

    GetCapsuleComponent()->InitCapsuleSize(42.f, 90.f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    //Assignar el arma para que no colisione
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Asignar defaults al componente de movimiento del personaje
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    MoveComp->bOrientRotationToMovement = true;
    MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
    MoveComp->MaxWalkSpeed = WALKSPEED;
    MoveComp->JumpZVelocity = JUMPZVELOCITY;
    MoveComp->AirControl = 0.35f;

	//Crear el brazo de la cámara y la cámara de seguimiento y asignar sus valores + adjuntarlos en la jerarquía de componentes
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APlayerCharacter::NotifyControllerChanged()
{
    Super::NotifyControllerChanged();

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (DefaultMappingContext)
            {
                Subsystem->AddMappingContext(DefaultMappingContext, 0);
            }
        }

		//Limitar pitch de la cámara para limitar la visión arriba/abajo del jugador
        if (PC->PlayerCameraManager)
        {
            PC->PlayerCameraManager->ViewPitchMin = -60.f;
            PC->PlayerCameraManager->ViewPitchMax = 30.f;
        }
    }
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
        {
            EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Move);
        }
        if (LookAction)
        {
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &APlayerCharacter::Look);
        }
        if (JumpAction)
        {
            EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }
		if (RunAction)
		{
			EIC->BindAction(RunAction, ETriggerEvent::Started, this, &APlayerCharacter::Run);
			EIC->BindAction(RunAction, ETriggerEvent::Completed, this, &APlayerCharacter::StopRun);
		}
        if (ShootAction)
        {
            EIC->BindAction(ShootAction, ETriggerEvent::Started, this, &APlayerCharacter::Shoot);
        }
    }
}

void APlayerCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D Input = Value.Get<FVector2D>();
    if (Controller == nullptr) return;

    const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
    const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(Forward, Input.Y);
    AddMovementInput(Right, Input.X);
}

void APlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxis = Value.Get<FVector2D>();

    AddControllerYawInput(LookAxis.X);
    AddControllerPitchInput(LookAxis.Y);
}


void APlayerCharacter::Run()
{
	GetCharacterMovement()->MaxWalkSpeed = RUNSPEED;
}

void APlayerCharacter::StopRun()
{
    GetCharacterMovement()->MaxWalkSpeed = WALKSPEED;
}

void APlayerCharacter::Shoot()
{
	if (FireMontage)
	{
		PlayAnimMontage(FireMontage);
	}
    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * ShootRange;

    FHitResult Hit;
    FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);   //Evitar tocar al jugador que dispara

    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	//De moment nomes es mostra per pantalla el nom de l'actor impactat, pero es podria fer que aquest actor rebi mal
    if (bHit)
    {
        UE_LOG(LogTemp, Log, TEXT("Impacte a: %s"), *GetNameSafe(Hit.GetActor()));
    }
}
// CMC Tutorial Copyright (c) 2023 Kyle Lautenbach
// Packed move data example adapted from SMN1 + SMN2 - Copyright (c) 2021 Reddy-dev (https://github.com/Reddy-dev/SMN2)
// Wall running example adapted from Zippy - Copyright (c) 2022 William (https://github.com/delgoodie/Zippy)

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TutCharacterMovementComponent.generated.h"

/////BEGIN Network Prediction Setup/////
#pragma region Network Prediction Setup

/*
* This initial section contains the boilerplate code you need to begin working with saved moves and network prediction.
* There is a neat little pattern here.
* You can create any internal variable within your child UCharacterMovementComponent, like bWantsToSprint.
* Then, you follow the pattern below where you ensure this value is tracked within the overridden Move Data AND Saved Move classes.
* bWantsToSprintMoveData and bWantsToSprintSaved are used for this purpose.
* We also use MovementFlagCustomMoveData as an alternative for reducing the bitrate (size of data being sent), demonstrated by the CFLAG_WantsToFly example. This uses bitshifting and bitflags to store 8 flags within 1 variable of size 8 bits.
* The pattern continues within the .cpp file.
* You will also see some important functions are overridden in each of these classes, which allow you to add in your custom variables and code and ensure the CMC is using your custom data.
*
* NOTE: Allowing clients to send arbitrary data can lead to cheating. It is better to work with *player intent* (inputs) rather than setting values directly,
* unless you don't care too much about cheating, such as a co-op game or casual multiplayer, or if you perform a sanity check on the incoming data on the server.
* Intent simply means gathering the player's input data, which we then use to evaluate the current state.
* For instance, we only allow characters to sprint when they move forward. If we just allow clients to tell the server what speed to use,
* they can bypass game mechanics and sprint in any direction, or at higher speeds than intended.
* As mentioned, you can either sanity check data or use the player input (intent) approach with pre-determined values that are common between server and clients.
* Of course, you may also want to include extra data, such as the custom LaunchVelocity example, but we will cover how this is handled by the server to prevent cheating. 
* It is best to watch the accompanying YouTube tutorial to better understand this code.
*/

//Network Move DATA
//This new Move Data system enables much more flexibility.
//Before, using the old compressed flags approach, we were limited to around 8 flags.
//This is usually not enough for modern games.
//But more than this, we can send almost any data type using Move Data within the corresponding Serialize function.
//As mentioned above, just be careful of UNSAFE variables. The client might be sending false info to cheat.
class FCustomNetworkMoveData : public FCharacterNetworkMoveData
{

public:

	typedef FCharacterNetworkMoveData Super;

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;

	virtual bool Serialize(UCharacterMovementComponent& CharacterMovement, FArchive& Ar, UPackageMap* PackageMap, ENetworkMoveType MoveType) override;

	//SAFE variables
	bool bWantsToSprintMoveData = false; 

	//UNSAFE variables
	float MaxCustomSpeedMoveData = 800.f;
	FVector LaunchVelocityCustomMoveData = FVector(0.f, 0.f, 0.f);
	
	//This bypasses the limitations of the typical compressed flags used in past versions of UE4. 
	//You would still use bitflags like this in games in order to improve network performance.
	//Imagine hundreds of clients sending this info every tick. Even a small saving can add up significantly, especially when considering server costs.
	//It is up to you to decide if you prefer sending bools like above or using the lightweight bitflag approach like below.
	//If you're making P2P games, casual online, or co-op vs AI, etc, then you might not care too much about maximising efficiency. The bool approach might be more readable.
	uint8 MovementFlagCustomMoveData = 0; 
};

class FCustomCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer
{

public:

	FCustomCharacterNetworkMoveDataContainer();

	FCustomNetworkMoveData CustomDefaultMoveData[3];
};

//Class FCustomSavedMove
class FCustomSavedMove : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;


	//All Saved Variables are placed here.
	//Boolean Flags
	bool bWantsToSprintSaved = false;

	//Not present in Move Data. This state is not sent over the network, it is inferred from running the internal CMC logic.
	//However, we still save it for replay purposes.
	bool bWallRunIsRightSaved = false; 

	//As you can see, our bWantsToFly variable is not present in MoveData or here in SavedMove like bWantsToSprint is. We use the info from MovementFlagCustomMoveData to change our state and save it as SavedMovementFlagCustom.
	//This is because Move Data is sent back and forth, much like the Compressed Flags were sent in the old system (before packed move data).
	//Thus, we aim to minimise the number of variables in our Move Data for the sake of network performance.
	//We can avoid having this variable here at all as our SavedMovementFlagCustom and normal MovementFlagCustom are both already present.
	//bool bWantsToFlySaved = false; //It would have otherwise been stored here like this if we were using the bool approach.

	//Contains our saved custom movement flags, like CFLAG_WantsToFly.
	uint8 SavedMovementFlagCustom = 0;
	
	//Variables
	float SavedMaxCustomSpeed = 800.f;
	FVector SavedLaunchVelocityCustom = FVector(0.f, 0.f, 0.f);	


	/** Returns a byte containing encoded special movement information (jumping, crouching, etc.)	 */
	virtual uint8 GetCompressedFlags() const override;

	/** Returns true if this move can be combined with NewMove for replication without changing any behaviour.
	* Just determines if any variables were modified between moves for optimisation purposes. If nothing changed, combine moves to save time.
	*/
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;

	/** Called to set up this saved move (when initially created) to make a predictive correction. */
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character& ClientData) override;
	/** Called before ClientUpdatePosition uses this SavedMove to make a predictive correction	 */
	virtual void PrepMoveFor(class ACharacter* Character) override;

	/** Clear saved move properties, so it can be re-used. */
	virtual void Clear() override;
};

//Class Prediction Data
class FCustomNetworkPredictionData_Client : public FNetworkPredictionData_Client_Character
{
public:
	FCustomNetworkPredictionData_Client(const UCharacterMovementComponent& ClientMovement);

	typedef FNetworkPredictionData_Client_Character Super;

	///Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
};

#pragma endregion
/////END Network Prediction Setup/////

/////BEGIN CMC Setup/////
#pragma region CMC Setup
/*
* This enum will house all of our extra movement modes.
* There can be a wide range of movement modes, depending on your game.
* Examples: Wall-running, grappling, sliding, parkouring, etc.
* In this tutorial, we will only cover wall running.
*/
UENUM(BlueprintType)
enum ECustomMovementMode {
	MOVE_CustomNone UMETA(Hidden),
	MOVE_WallRunning   UMETA(DisplayName = "WallRunning"),
};

/* Our optimised movement flag container.
* This uses bitshifting to pack a whole lot of extra flags into one tiny container, which can lower your bitrate usage.
* Sending larger data types via packed moves can negatively impact network performance.
* However, every project is different. You might prefer the readability and ease of use of using normal data types like bools (as demonstrated).
* But this may not cut it in a networked environment that requires minimal network usage (like saving on server bandwidth costs in AA to AAA games). 
*/
UENUM(BlueprintType, Meta = (Bitflags))
enum class EMovementFlag : uint8
{
	NONE = 0 UMETA(Hidden), //Requires a 0 entry as default when initialised, but we hide it from BP.

	CFLAG_WantsToFly = 1 << 0,

	CFLAG_OtherFlag1 = 1 << 1, //This could be used as CFLAG_WantsToSprint and follow the same logic as CFLAG_WantsToFly. Instead, this tutorial demonstrates both approaches for you to decide your preference. But keep your project requirements in mind in regards to bitrate.
	CFLAG_OtherFlag2 = 1 << 2,
	CFLAG_OtherFlag3 = 1 << 3,
};

/**
 * Forward-declare our imported classes that are referenced here.
 * We do this to reduce header file overhead. Too many includes in a header file that is then included in subsequent header files can bog down compile times.
 * Also, most classes that include this header may not need all of the references contained in the .cpp file.
 * Thus, we declare external classes here in the header and include required headers in the .cpp file.
 * There are certainly exceptions to this rule, such as reducing the need to include common classes by hosting them within a certain header file.
 */
class AMyCustomCharacter;

/**
 *
 */
UCLASS()
class TUTORIALRESEARCH_API UTutCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:

	UTutCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	//Reference to our network prediction buddy, the custom saved move class created above. 
	friend class FCustomSavedMove;

	/////BEGIN Sprinting/////

	/*
	* Sprinting is different from other movement types. We don't need to add a whole new movement mode as we are simply adjusting the max movement speed.
	* If it requires additional logic in your game, you can certainly create a sprinting movement mode, but it isn't necessary for most use cases.
	* We will still be using the PhysWalking logic in this implementation.
	* Thus, this can be seen as a movement modifier, not a new movement type.
	*/

	/*
	* We create two variables here. This one tracks player intent, such as holding the sprint button down or toggling it.
	* While the player intends to sprint, the movement can use this information to trigger different logic. We can even re-use it to act as an indicator to start wall-running, for instance.
	* In this case, if the player intends to sprint but isn't sprinting, we trigger sprinting.
	* Similarly, if the sprinting is interrupted, we would want to resume sprinting as soon as possible.
	* The implementation of this logic may differ in a GAS (Gameplay Ability System) setup where this variable will be controlled by Gameplay Abilities (GA) and Gameplay Effects (GE).
	* We do not replicate the variable as it's only really relevant to the owning client and the server.
	* The network prediction setup will ensure sync between owning client and server.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Sprinting")
	bool bWantsToSprint;

	/*
	* This variable controls the actual sprinting logic. If it's true, the character will be moving at a higher velocity.
	* It can be used as an internal CMC variable to track a gameplay tag that is applied/removed by GAS (e.g. State.Movement.Sprinting or State.Buff.Sprinting, depending on preference and design).
	* But in this basic tutorial, we will use it directly.
	* This is replicated as we want this variable to propagate down to simulated proxies (other clients).
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Sprinting")
	bool bIsSprinting;

	/*
	* The current maximum speed that the character can run.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Sprinting")
	float CustomMaxSpeed;

	/*
	* A simple function to determine if the character is able to sprint in its current state.
	* This function does not factor in external state information at the moment.
	* If used with GAS, this could be a helper function to determine if the Sprint Ability can be applied or not, alongside the use of gameplay tags and other checks.
	*/
	UFUNCTION(BlueprintCallable, Category = "Sprinting")
	virtual bool CanSprint() const;

	/////END Sprinting/////

	/////BEGIN Custom Movement/////
#pragma region Custom Movement

	/*
	* We override the custom phys function to enable us to execute our various new movement modes.
	* While you could override other functions to achieve the same effect, this is a common workflow for networked CMC movement.
	*/
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	//Notice how this variable is declared. This allows BP to allow you to work with bit flags.
	UPROPERTY(EditAnywhere, Category = TestBitflag, meta = (Bitmask, BitmaskEnum = "EMovementFlag"))
	uint8 MovementFlagCustom = 0;

	//We also want a way to test if a flag is active in C++ or BP.
	UFUNCTION(BlueprintCallable)
	bool IsFlagActive(UPARAM(meta = (Bitmask, BitmaskEnum = EMovementFlag)) uint8 TestFlag) const {return (MovementFlagCustom & TestFlag); }

	UFUNCTION(BlueprintCallable)
	virtual void ActivateMovementFlag(UPARAM(meta = (Bitmask, BitmaskEnum = EMovementFlag)) uint8 FlagToActivate);

	UFUNCTION(BlueprintCallable)
	virtual void ClearMovementFlag(UPARAM(meta = (Bitmask, BitmaskEnum = EMovementFlag)) uint8 FlagToClear);

#pragma region Flying


	/*
	* This is different from our other replicated simulated proxy variables (this is not replicated).
	* Because we have an enter/exit mode when the movement mode changes, our bIsFlying bool is automatically updated on simulated proxies when they apply the new movement mode.
	* Of course, we might not want some logic to run on sim proxies, but you can simply perform checks for that. 
	* Just keep track of where code will and will not (or should not) be run on simulated proxies.
	* Working around movement mode changes works well for setting extra variables.
	* You could make this variable protected/private and expose an accessor method to BP, but this works just fine. Even something like MovementMode is publicly accessible in the parent classes.
	* Pick a pattern or stick to coding conventions adopted by your team/company/etc. 
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flying")
	bool bIsFlying;

	//As part of our new bit flags, we have added a flying flag, similar to bWantsToSprint.
	//This function will be called in OnMovementUpdated to change movement modes if able.
	UFUNCTION(BlueprintCallable)
	virtual bool CanFly() const;

	/*
	* This is a great way to expose easy helper functions to BP.
	* Instead of having to call IsFlagActive directly for common checks, you can also add in helpers like this. 
	* But IsFlagActive is already quite easy to use in BP because of how it was declared, so these helpers are still completely optional. 
	* It is included here as an example.
	*/
	UFUNCTION(BlueprintCallable)
	virtual bool DoesCharacterWantToFly() const {return IsFlagActive((uint8)EMovementFlag::CFLAG_WantsToFly);}

/*We make our enter/exit functions private/protected so people don't accidentally call them when trying to start/end wall running or flying.
* We should ALWAYS use SetMovementMode instead based on our design pattern.
* Always consider how your function names and their visibility can accidentally mislead your team members.
* private/protected exists for this reason. It is also best to have accessible documentation for team members to reference and understand your design pattern.
*/
protected:
	/*
	* Here we have our ENTER/EXIT functions for the wall running movement mode.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Flying")
		void EnterFlying();
	virtual void EnterFlying_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Flying")
		void ExitFlying();
	virtual void ExitFlying_Implementation();

#pragma endregion

#pragma region Replicated Launch

public:
	/*
	* Allows us to apply a replicated launch, effectively adding a "boop" mechanic or a predicted jump pad, etc.
	* This can be considered an UNSAFE variable, though. The player could use this to cheat, so we need to make other checks before executing the launch.
	*/
	UFUNCTION(BlueprintCallable, Category = "Launching")
		void LaunchCharacterReplicated(FVector NewLaunchVelocity, bool bXYOverride, bool bZOverride);
	//Network predicted variable.
	FVector LaunchVelocityCustom;

	//Override the parent handle launch to check our incoming launch requests.
	virtual bool HandlePendingLaunch() override;

#pragma endregion

/////BEGIN Wall-Running/////
#pragma region Wall Running
	/*
	* Wall-running has entirely new logic that differs from any of the existing movement modes (PhysWalking, PhysFlying, etc).
	* As a result, we need to create a new movement mode to switch into.
	* Fortunately, we can use a lot of the existing CMC code to help create our solution.
	* This is far more than just a modifier, thus we create a movement mode.
	*/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Wall Running")
	bool bWallRunIsRight;

	// Wall Run Variables
	UPROPERTY(EditDefaultsOnly) float MinWallRunSpeed = 200.f;
	UPROPERTY(EditDefaultsOnly) float MaxWallRunSpeed = 800.f;
	UPROPERTY(EditDefaultsOnly) float MaxVerticalWallRunSpeed = 200.f;
	UPROPERTY(EditDefaultsOnly) float WallRunPullAwayAngle = 75;
	UPROPERTY(EditDefaultsOnly) float WallAttractionForce = 200.f;
	UPROPERTY(EditDefaultsOnly) float MinWallRunHeight = 50.f;
	UPROPERTY(EditDefaultsOnly) UCurveFloat* WallRunGravityScaleCurve;
	UPROPERTY(EditDefaultsOnly) float WallJumpForce = 300.f;
	
	
	UFUNCTION(BlueprintPure) bool IsWallRunning() const { return IsCustomMovementMode(MOVE_WallRunning); }
	
	// Returns true if the movement mode is custom and matches the provided custom movement mode
	bool IsCustomMovementMode(uint8 TestCustomMovementMode) const;	

	/*
	* We override the landing function to reset some stateful variables upon landing on a suitable surface.
	*/
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;

protected: 
	//Helper functions
	float OwnerCapsuleRadius() const;
	float OwnerCapsuleHalfHeight() const;

	/*
	* Attempt to initiate wall running.
	*/
	virtual bool TryWallRun();
	/*
	* The actual processing and execution of the wall running movement mode.
	* Similar to PhysFlying, etc.
	*/
	virtual void PhysWallRun(float deltaTime, int32 Iterations);

	/*
	* Here we have our ENTER/EXIT functions for the wall running movement mode.
	*/
	UFUNCTION(BlueprintNativeEvent, Category = "Wall Running")
	void EnterWallRun();
	virtual void EnterWallRun_Implementation();

	UFUNCTION(BlueprintNativeEvent, Category = "Wall Running")
	void ExitWallRun();
	virtual void ExitWallRun_Implementation();

#pragma endregion
/////END Wall-Running/////


#pragma endregion
/////END Custom Movement/////

protected:

	/** Character movement component belongs to */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<AMyCustomCharacter> CustomCharacter;

public:

	//BEGIN UActorComponent Interface
	virtual void BeginPlay() override;
	//END UActorComponent Interface

	//BEGIN UMovementComponent Interface
	virtual float GetMaxSpeed() const override;

	/** Update the character state in PerformMovement right before doing the actual position change */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	/** Update the character state in PerformMovement after the position change. Some rotation updates happen after this. */
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

	/** Consider this to be the final chance to change logic before the next tick. It can be useful to defer certain actions to the next tick. */
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	/** Override jump behaviour to help us create custom logic. */
	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves) override;
	//END UMovementComponent Interface

	//Replication. Boilerplate function that handles replicated variables. 
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	/** Called after MovementMode has changed. Base implementation performs special handling for starting certain modes, then notifies the CharacterOwner. 
	*	We update it to become our central movement mode switching function. All enter/exit functions should be stored here or in SetMovementMode.
	*/
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

public:

/////BEGIN Networked Movement/////
#pragma region Networked Movement Setup
	//New Move Data Container
	FCustomCharacterNetworkMoveDataContainer MoveDataContainer;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;	
	virtual void MoveAutonomous(float ClientTimeStamp, float DeltaTime, uint8 CompressedFlags, const FVector& NewAccel) override;


	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;

#pragma endregion
/////END Networked Movement/////
};

#pragma endregion
/////END CMC Setup/////

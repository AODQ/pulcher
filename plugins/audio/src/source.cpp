#include <pulcher-audio/system.hpp>
#include <pulcher-core/scene-bundle.hpp>
#include <pulcher-plugin/plugin.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/consts.hpp>
#include <pulcher-util/log.hpp>

#include <fmod.hpp>
#include <fmod_errors.h>

namespace {

FMOD_SYSTEM * fmodSystem = nullptr;
std::array<FMOD_SOUND *, 2> fmodSoundSpawn = { nullptr };
std::array<FMOD_SOUND *, 5> fmodSoundTaunt = { nullptr };
std::array<FMOD_SOUND *, 3> fmodSoundJump = { nullptr };
std::array<FMOD_SOUND *, 3> fmodSoundSlide = { nullptr };
std::array<FMOD_SOUND *, 4> fmodSoundVolniasFire = { nullptr };
std::array<FMOD_SOUND *, 3> fmodSoundVolniasHit = { nullptr };
std::array<FMOD_SOUND *, 5> fmodSoundVolnias = { nullptr };
std::array<FMOD_SOUND *, 3> fmodSoundStep = { nullptr };
std::array<FMOD_SOUND *, 3> fmodSoundDash = { nullptr };
std::array<FMOD_SOUND *, 3> fmodSoundLand = { nullptr };
std::array<FMOD_SOUND *, 3> fmodSoundLandEnv = { nullptr };
std::array<FMOD_SOUND *, Idx(pul::core::PickupType::Size)> fmodSoundPickup =
  { nullptr };

#define FMOD_ASSERT(X, ...) \
  if (auto result = X; result != FMOD_OK) { \
    spdlog::critical( \
      "FMOD error; {}@{}; '{}': {}" \
    , __FILE__, __LINE__, #X , FMOD_ErrorString(result) \
    ); \
    __VA_ARGS__ \
  }

void InitializeSystem() {

  FMOD_ASSERT(FMOD_System_Create(&fmodSystem), return;);
  FMOD_ASSERT(FMOD_System_Init(fmodSystem, 512, FMOD_INIT_NORMAL, 0), return;);

  FMOD_SOUNDGROUP * soundGroup;
  FMOD_ASSERT(
    FMOD_System_GetMasterSoundGroup(fmodSystem, &soundGroup), return;
  );

  FMOD_SoundGroup_SetVolume(soundGroup, 0.1f);
}

}

extern "C" {

PUL_PLUGIN_DECL void Audio_LoadAudio(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
  ::InitializeSystem();

  //-- pickups
  for (auto pickup : std::vector<std::pair<int, char const *>> {
    { Idx(pul::core::PickupType::HealthLarge),  "pick-health3" }
  , { Idx(pul::core::PickupType::HealthMedium), "pick-health2" }
  , { Idx(pul::core::PickupType::HealthSmall),  "pick-health1" }
  , { Idx(pul::core::PickupType::ArmorLarge),   "pick-armor3"  }
  , { Idx(pul::core::PickupType::ArmorMedium),  "pick-armor2"  }
  , { Idx(pul::core::PickupType::ArmorSmall),   "pick-armor1"  }
  , { Idx(pul::core::PickupType::Weapon), "pick-weapon" }
  }) {
    auto const str =
        std::string{"assets/base/audio/sfx/pickups/"}
      + std::get<1>(pickup) + ".wav"
    ;

    spdlog::debug("str {}", str);
    FMOD_ASSERT(
      FMOD_System_CreateSound(
        ::fmodSystem
      , str.c_str()
      , FMOD_LOOP_OFF | FMOD_2D
      , nullptr
      , &::fmodSoundPickup[std::get<0>(pickup)]
      ), ;
    );
  }
  //-- pickups

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/misc/player-spawn.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundSpawn[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/pickups/pick-health2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundSpawn[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/land-normal1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundLandEnv[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/land-normal1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundLandEnv[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/land-normal1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundLandEnv[2]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/land1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundLand[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/land2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundLand[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/land3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundLand[2]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/dash1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundDash[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/dash2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundDash[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/dash3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundDash[2]
    ), ;
  );


  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/taunt1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundTaunt[0]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/taunt2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundTaunt[1]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/taunt3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundTaunt[2]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/taunt4.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundTaunt[3]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/taunt5.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundTaunt[4]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-charge1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolnias[0]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-charge2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolnias[1]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-end.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolnias[2]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-prefire1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolnias[3]
    ), ;
  );
  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-prefire2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolnias[4]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-fire1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolniasFire[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-fire2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolniasFire[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-fire3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolniasFire[2]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-fire4.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolniasFire[3]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-hit1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolniasHit[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-hit2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolniasHit[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/weapons/volnias-hit3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundVolniasHit[2]
    ), ;
  );


  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/step-normal1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundStep[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/step-normal2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundStep[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/step-normal3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundStep[2]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/slide1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundSlide[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/slide2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundSlide[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/player/slide3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundSlide[2]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/jump1.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundJump[0]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/jump2.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundJump[1]
    ), ;
  );

  FMOD_ASSERT(
    FMOD_System_CreateSound(
      ::fmodSystem
    , "assets/base/audio/sfx/characters/nygelstromn/jump3.wav"
    , FMOD_LOOP_OFF | FMOD_2D
    , nullptr
    , &::fmodSoundJump[2]
    ), ;
  );
}

PUL_PLUGIN_DECL void Audio_Shutdown(
  pul::core::SceneBundle &
) {
  FMOD_System_Release(::fmodSystem);
  ::fmodSystem = nullptr;
}

PUL_PLUGIN_DECL void Audio_Update(
  pul::plugin::Info const &, pul::core::SceneBundle & scene
) {
  FMOD_System_Update(::fmodSystem);

  auto & audioSystem = scene.AudioSystem();

  if (audioSystem.playerJumped) {
    static size_t it = 0;
    ++ it;
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundJump[it % 3]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }

  if (audioSystem.playerTaunted) {
    static size_t it = 0;
    ++ it;
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundTaunt[it % 5]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }

  if (audioSystem.volniasFire != -1ul) {
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundVolniasFire[audioSystem.volniasFire]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }

  if (audioSystem.volniasHit) {
    static size_t it = 0;
    ++ it;
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundVolniasHit[it % 3]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }


  if (audioSystem.volniasChargePrimary) {
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem, ::fmodSoundVolnias[0], nullptr, false, nullptr
      ),
      ;
    );
  }
  if (audioSystem.volniasChargeSecondary) {
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem, ::fmodSoundVolnias[1], nullptr, false, nullptr
      ),
      ;
    );
  }
  if (audioSystem.volniasEndPrimary) {
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem, ::fmodSoundVolnias[2], nullptr, false, nullptr
      ),
      ;
    );
  }
  if (audioSystem.volniasPrefirePrimary) {
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem, ::fmodSoundVolnias[3], nullptr, false, nullptr
      ),
      ;
    );
  }
  if (audioSystem.volniasPrefireSecondary) {
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem, ::fmodSoundVolnias[4], nullptr, false, nullptr
      ),
      ;
    );
  }

  if (audioSystem.playerStepped) {
    static size_t it = 0;
    ++ it;
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundStep[it % 3]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }

  if (audioSystem.playerSlided) {
    static size_t it = 0;
    ++ it;
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundSlide[it % 3]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }

  if (audioSystem.playerDashed) {
    static size_t it = 0;
    ++ it;
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundDash[it % 3]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }

  if (audioSystem.playerLanded) {
    static size_t it = 0;
    ++ it;
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundLand[it % 3]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }

  for (size_t idx = 0ul; idx < audioSystem.pickup.size(); ++ idx) {
    auto & pickup = audioSystem.pickup[idx];
    if (pickup) {
      FMOD_ASSERT(
        FMOD_System_PlaySound(
          ::fmodSystem
        , ::fmodSoundPickup[idx]
        , nullptr
        , false
        , nullptr
        ),
        ;
      );
    }
    pickup = false;
  }

  if (audioSystem.envLanded < 3) {
    FMOD_ASSERT(
      FMOD_System_PlaySound(
        ::fmodSystem
      , ::fmodSoundLandEnv[audioSystem.envLanded]
      , nullptr
      , false
      , nullptr
      ),
      ;
    );
  }


  audioSystem.playerJumped = false;
  audioSystem.playerTaunted = false;
  audioSystem.volniasHit = false;
  audioSystem.volniasChargePrimary = false;
  audioSystem.volniasChargeSecondary = false;
  audioSystem.volniasEndPrimary = false;
  audioSystem.volniasPrefirePrimary = false;
  audioSystem.volniasPrefireSecondary = false;
  audioSystem.volniasFire = -1ul;
  audioSystem.playerSlided = false;
  audioSystem.playerStepped = false;
  audioSystem.playerDashed = false;
  audioSystem.playerLanded = false;
  audioSystem.envLanded = -1ul;
}

PUL_PLUGIN_DECL void Audio_UiRender(
  pul::plugin::Info const &, pul::core::SceneBundle &
) {
}

}

#include <plugin-entity/config.hpp>

#include <pulcher-gfx/imgui.hpp>
#include <pulcher-util/log.hpp>

#include <cjson/cJSON.h>
#include <fstream>

// this file is just configs so formatting it is unnecssary

//------------------------------------------------------------------------------
namespace plugin::config {
  namespace volnias::primary {
    int32_t & ChargeupPreBeginThreshold() {
      static int32_t chargeupPreBeginThreshold = 100;
      return chargeupPreBeginThreshold;
    }

    int32_t & ChargeupBeginThreshold() {
      static int32_t chargeupBeginThreshold = 1000;
      return chargeupBeginThreshold;
    }

    int32_t & ChargeupDelta() {
      static int32_t chargeupDelta = 100;
      return chargeupDelta;
    }

    int32_t & ChargeupTimerEnd() {
      static int32_t chargeupTimerEnd = 400;
      return chargeupTimerEnd;
    }

    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 300;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    int32_t & ProjectileDamage() {
      static int32_t projectileDamage = 10;
      return projectileDamage;
    }

    float & Knockback() {
      static float knockback = 0.0f;
      return knockback;
    }

  }

  namespace volnias::secondary {
    int32_t & MaxChargedShots() {
      static int32_t maxChargedShots = 5;
      return maxChargedShots;
    }

    int32_t & ChargeupDelta() {
      static int32_t chargeupDelta = 600;
      return chargeupDelta;
    }

    int32_t & ChargeupMaxThreshold() {
      static int32_t chargeupMaxThreshold = 6000;
      return chargeupMaxThreshold;
    }

    int32_t & ChargeupTimerStart() {
      static int32_t chargeupTimerStart = 400;
      return chargeupTimerStart;
    }

    int32_t & DischargeDelta() {
      static int32_t dischargeDelta = 40;
      return dischargeDelta;
    }


    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 300;
      return dischargeCooldown;
    }

  }

  namespace grannibal::primary {
    int32_t & MuzzleTrailTimer() {
      static int32_t muzzleTrailTimer = 70;
      return muzzleTrailTimer;
    }

    int32_t & MuzzleTrailParticles() {
      static int32_t muzzleTrailParticles = 4;
      return muzzleTrailParticles;
    }

    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    int32_t & ProjectileExplosionRadius() {
      static int32_t projectileExplosionRadius = 96;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }

  }

  namespace grannibal::secondary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    int32_t & ProjectileExplosionRadius() {
      static int32_t projectileExplosionRadius = 96;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }


    int32_t & Bounces() {
      static int32_t bounces = 2u;
      return bounces;
    }

    float & ProjectileVelocityFriction() {
      static float projectileVelocityFriction = 0.0f;
      return projectileVelocityFriction;
    }

  }

  namespace dopplerBeam::primary {
    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    int32_t & ProjectileDamage() {
      static int32_t projectileDamage = 10;
      return projectileDamage;
    }

    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 150;
      return dischargeCooldown;
    }

  }

  namespace dopplerBeam::secondary {
    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    int32_t & ProjectileDamage() {
      static int32_t projectileDamage = 10;
      return projectileDamage;
    }

    std::vector<float> & ShotPattern() {
      static std::vector<float> shotPattern = { -0.1f, 0.0f, +0.1 };
      return shotPattern;
    }

    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

  }

  namespace pericaliya::primary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    int32_t & ProjectileExplosionRadius() {
      static int32_t projectileExplosionRadius = 96;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }

  }

  namespace pericaliya::secondary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    int32_t & ProjectileExplosionRadius() {
      static int32_t projectileExplosionRadius = 96;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }


    int32_t & RedirectionMinimumThreshold() {
      static int32_t redirectionMinimumThreshold = 100;
      return redirectionMinimumThreshold;
    }


    std::vector<float> & ShotPattern() {
      static std::vector<float> shotPattern = { -0.2f, 0.0f, +0.2 };
      return shotPattern;
    }

  }

  namespace zeusStinger::primary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    int32_t & ProjectileDamage() {
      static int32_t projectileDamage = 80;
      return projectileDamage;
    }

  }

  namespace zeusStinger::secondary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileVelocityFriction() {
      static float projectileVelocityFriction = 0.0f;
      return projectileVelocityFriction;
    }

    int32_t & ProjectileLifetime() {
      static int32_t projectileLifetime = 4000;
      return projectileLifetime;
    }

    int32_t & ProjectileExplosionRadius() {
      static int32_t projectileExplosionRadius = 96;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }


    int32_t & RedirectionMinimumThreshold() {
      static int32_t redirectionMinimumThreshold = 100;
      return redirectionMinimumThreshold;
    }


    std::vector<float> & ShotPattern() {
      static std::vector<float> shotPattern = { -0.2f, 0.0f, +0.2 };
      return shotPattern;
    }

  }

  namespace badFetus::primary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = -0.0f;
      return projectileForce;
    }

    int32_t & ProjectileCooldown() {
      static int32_t projectileCooldown = 100;
      return projectileCooldown;
    }

    int32_t & ProjectileDamage() {
      static int32_t projectileDamage = 80;
      return projectileDamage;
    }

  }

  namespace badFetus::secondary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileVelocityFriction() {
      static float projectileVelocityFriction = 0.0f;
      return projectileVelocityFriction;
    }

    int32_t & ProjectileLifetime() {
      static int32_t projectileLifetime = 4000;
      return projectileLifetime;
    }

    int32_t & ProjectileExplosionRadius() {
      static int32_t projectileExplosionRadius = 96;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }

  }

  namespace badFetus::combo {
    float & VelocityFriction() {
      static float velocityFriction = 0.0f;
      return velocityFriction;
    }

    int32_t & ExplosionRadius() {
      static int32_t explosionRadius = 64;
      return explosionRadius;
    }

    float & ExplosionForce() {
      static float explosionForce = -2.0f;
      return explosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 10;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 60;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 20;
      return projectileDirectDamage;
    }

  }

  namespace manshredder::primary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 80;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = 0.0f;
      return projectileForce;
    }

    int32_t & ProjectileCooldown() {
      static int32_t projectileCooldown = 80;
      return projectileCooldown;
    }

    int32_t & ProjectileDamage() {
      static int32_t projectileDamage = 80;
      return projectileDamage;
    }

    int32_t & ProjectileDistance() {
      static int32_t projectileDistance = 32;
      return projectileDistance;
    }

  }

  namespace manshredder::secondary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    int32_t & ProjectileExplosionRadius() {
      static int32_t projectileExplosionRadius = 96;
      return projectileExplosionRadius;
    }

    float & ProjectileExplosionForce() {
      static float projectileExplosionForce = 5.0f;
      return projectileExplosionForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }

  }

  namespace wallbanger::primary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileVelocity() {
      static float projectileVelocity = 5.0f;
      return projectileVelocity;
    }

    float & ProjectileForce() {
      static float projectileForce = 5.0f;
      return projectileForce;
    }

    int32_t & ProjectileSplashDamageMin() {
      static int32_t projectileSplashDamageMin = 50;
      return projectileSplashDamageMin;
    }

    int32_t & ProjectileSplashDamageMax() {
      static int32_t projectileSplashDamageMax = 20;
      return projectileSplashDamageMax;
    }

    int32_t & ProjectileDirectDamage() {
      static int32_t projectileDirectDamage = 80;
      return projectileDirectDamage;
    }

  }

  namespace wallbanger::secondary {
    int32_t & DischargeCooldown() {
      static int32_t dischargeCooldown = 1000;
      return dischargeCooldown;
    }

    float & ProjectileForce() {
      static float projectileForce = 0.0f;
      return projectileForce;
    }

    int32_t & ProjectileDamage() {
      static int32_t projectileDamage = 80;
      return projectileDamage;
    }

  }

  namespace weapon {
    int32_t & WeaponSwitchCooldown() {
      static int32_t weaponSwitchCooldown = 300;
      return weaponSwitchCooldown;
    }
  }
}

namespace {

cJSON * LoadJsonFile(std::string const & filename) {
  // load file
  auto file = std::ifstream{filename};
  if (file.eof() || !file.good()) {
    spdlog::error("could not load spritesheet '{}'", filename);
    return nullptr;
  }

  auto str =
    std::string {
      std::istreambuf_iterator<char>(file)
    , std::istreambuf_iterator<char>()
    };

  auto fileDataJson = cJSON_Parse(str.c_str());

  if (fileDataJson == nullptr) {
    spdlog::critical(
      " -- failed to parse json for '{}'; '{}'"
    , filename
    , cJSON_GetErrorPtr()
    );
  }

  return fileDataJson;
}

void ApplyToConfig(
  std::function<void(float & value, std::string const & label)> fnFloat
, std::function<void(int32_t & value, std::string const & label)> fnInt
, std::function<void(std::vector<float> & value, std::string const & label)>
    fnVectorFloat
) {
  {
    namespace config = plugin::config::volnias::primary;
    fnInt(config::ChargeupPreBeginThreshold(), "volnias::primary::chargeupPreBeginThreshold");
    fnInt(config::ChargeupBeginThreshold(), "volnias::primary::chargeupBeginThreshold");
    fnInt(config::ChargeupDelta(), "volnias::primary::chargeupDelta");
    fnInt(config::ChargeupTimerEnd(), "volnias::primary::chargeupTimerEnd");
    fnInt(config::DischargeCooldown(), "volnias::primary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "volnias::primary::projectileVelocity");
    fnFloat(config::ProjectileForce(), "volnias::primary::projectileForce");
    fnInt(config::ProjectileDamage(), "volnias::primary::projectileDamage");
    fnFloat(config::Knockback(), "volnias::primary::knockback");
  }

  {
    namespace config = plugin::config::volnias::secondary;
    fnInt(config::MaxChargedShots(), "volnias::secondary::maxChargedShots");
    fnInt(config::ChargeupDelta(), "volnias::secondary::chargeupDelta");
    fnInt(config::ChargeupMaxThreshold(), "volnias::secondary::chargeupMaxThreshold");
    fnInt(config::ChargeupTimerStart(), "volnias::secondary::chargeupTimerStart");
    fnInt(config::DischargeDelta(), "volnias::secondary::dischargeDelta");

    fnInt(config::DischargeCooldown(), "volnias::secondary::dischargeCooldown");
  }

  {
    namespace config = plugin::config::grannibal::primary;
    fnInt(config::MuzzleTrailTimer(), "grannibal::primary::muzzleTrailTimer");
    fnInt(config::MuzzleTrailParticles(), "grannibal::primary::muzzleTrailParticles");

    fnInt(config::DischargeCooldown(), "grannibal::primary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "grannibal::primary::projectileVelocity");
    fnInt(config::ProjectileExplosionRadius(), "grannibal::primary::projectileExplosionRadius");
    fnFloat(config::ProjectileExplosionForce(), "grannibal::primary::projectileExplosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "grannibal::primary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "grannibal::primary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "grannibal::primary::projectileDirectDamage");
  }

  {
    namespace config = plugin::config::grannibal::secondary;
    fnInt(config::DischargeCooldown(), "grannibal::secondary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "grannibal::secondary::projectileVelocity");
    fnInt(config::ProjectileExplosionRadius(), "grannibal::secondary::projectileExplosionRadius");
    fnFloat(config::ProjectileExplosionForce(), "grannibal::secondary::projectileExplosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "grannibal::secondary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "grannibal::secondary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "grannibal::secondary::projectileDirectDamage");

    fnInt(config::Bounces(), "grannibal::secondary::bounces");
    fnFloat(config::ProjectileVelocityFriction(), "grannibal::secondary::projectileVelocityFriction");
  }

  {
    namespace config = plugin::config::dopplerBeam::primary;
    fnFloat(config::ProjectileVelocity(), "dopplerBeam::primary::projectileVelocity");
    fnFloat(config::ProjectileForce(), "dopplerBeam::primary::projectileForce");
    fnInt(config::ProjectileDamage(), "dopplerBeam::primary::projectileDamage");
    fnInt(config::DischargeCooldown(), "dopplerBeam::primary::dischargeCooldown");
  }

  {
    namespace config = plugin::config::dopplerBeam::secondary;
    fnFloat(config::ProjectileVelocity(), "dopplerBeam::secondary::projectileVelocity");
    fnFloat(config::ProjectileForce(), "dopplerBeam::secondary::projectileForce");
    fnInt(config::ProjectileDamage(), "dopplerBeam::secondary::projectileDamage");
    fnVectorFloat(config::ShotPattern(), "dopplerBeam::secondary::shotPattern");
    fnInt(config::DischargeCooldown(), "dopplerBeam::secondary::dischargeCooldown");
  }

  {
    namespace config = plugin::config::pericaliya::primary;
    fnInt(config::DischargeCooldown(), "pericaliya::primary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "pericaliya::primary::projectileVelocity");
    fnInt(config::ProjectileExplosionRadius(), "pericaliya::primary::projectileExplosionRadius");
    fnFloat(config::ProjectileExplosionForce(), "pericaliya::primary::projectileExplosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "pericaliya::primary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "pericaliya::primary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "pericaliya::primary::projectileDirectDamage");
  }

  {
    namespace config = plugin::config::pericaliya::secondary;
    fnInt(config::DischargeCooldown(), "pericaliya::secondary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "pericaliya::secondary::projectileVelocity");
    fnInt(config::ProjectileExplosionRadius(), "pericaliya::secondary::projectileExplosionRadius");
    fnFloat(config::ProjectileExplosionForce(), "pericaliya::secondary::projectileExplosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "pericaliya::secondary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "pericaliya::secondary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "pericaliya::secondary::projectileDirectDamage");

    fnInt(config::RedirectionMinimumThreshold(), "pericaliya::secondary::redirectionMinimumThreshold");

    fnVectorFloat(config::ShotPattern(), "pericaliya::secondary::shotPattern");
  }

  {
    namespace config = plugin::config::zeusStinger::primary;
    fnInt(config::DischargeCooldown(), "zeusStinger::primary::dischargeCooldown");
    fnFloat(config::ProjectileForce(), "zeusStinger::primary::projectileForce");
    fnInt(config::ProjectileDamage(), "zeusStinger::primary::projectileDamage");
  }

  {
    namespace config = plugin::config::zeusStinger::secondary;
    fnInt(config::DischargeCooldown(), "zeusStinger::secondary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "zeusStinger::secondary::projectileVelocity");
    fnFloat(config::ProjectileVelocityFriction(), "zeusStinger::secondary::projectileVelocityFriction");
    fnInt(config::ProjectileLifetime(), "zeusStinger::secondary::projectileLifetime");
    fnInt(config::ProjectileExplosionRadius(), "zeusStinger::secondary::projectileExplosionRadius");
    fnFloat(config::ProjectileExplosionForce(), "zeusStinger::secondary::projectileExplosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "zeusStinger::secondary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "zeusStinger::secondary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "zeusStinger::secondary::projectileDirectDamage");

    fnInt(config::RedirectionMinimumThreshold(), "zeusStinger::secondary::redirectionMinimumThreshold");

    fnVectorFloat(config::ShotPattern(), "zeusStinger::secondary::shotPattern");
  }

  {
    namespace config = plugin::config::badFetus::primary;
    fnInt(config::DischargeCooldown(), "badFetus::primary::dischargeCooldown");
    fnFloat(config::ProjectileForce(), "badFetus::primary::projectileForce");
    fnInt(config::ProjectileCooldown(), "badFetus::primary::projectileCooldown");
    fnInt(config::ProjectileDamage(), "badFetus::primary::projectileDamage");
  }

  {
    namespace config = plugin::config::badFetus::secondary;
    fnInt(config::DischargeCooldown(), "badFetus::secondary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "badFetus::secondary::projectileVelocity");
    fnFloat(config::ProjectileVelocityFriction(), "badFetus::secondary::projectileVelocityFriction");
    fnInt(config::ProjectileLifetime(), "badFetus::secondary::projectileLifetime");
    fnInt(config::ProjectileExplosionRadius(), "badFetus::secondary::projectileExplosionRadius");
    fnFloat(config::ProjectileExplosionForce(), "badFetus::secondary::projectileExplosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "badFetus::secondary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "badFetus::secondary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "badFetus::secondary::projectileDirectDamage");
  }

  {
    namespace config = plugin::config::badFetus::combo;
    fnFloat(config::VelocityFriction(), "badFetus::combo::velocityFriction");
    fnInt(config::ExplosionRadius(), "badFetus::combo::explosionRadius");
    fnFloat(config::ExplosionForce(), "badFetus::combo::explosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "badFetus::combo::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "badFetus::combo::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "badFetus::combo::projectileDirectDamage");
  }

  {
    namespace config = plugin::config::manshredder::primary;
    fnInt(config::DischargeCooldown(), "manshredder::primary::dischargeCooldown");
    fnFloat(config::ProjectileForce(), "manshredder::primary::projectileForce");
    fnInt(config::ProjectileCooldown(), "manshredder::primary::projectileCooldown");
    fnInt(config::ProjectileDamage(), "manshredder::primary::projectileDamage");
    fnInt(config::ProjectileDistance(), "manshredder::primary::projectileDistance");
  }

  {
    namespace config = plugin::config::manshredder::secondary;
    fnInt(config::DischargeCooldown(), "manshredder::secondary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "manshredder::secondary::projectileVelocity");
    fnInt(config::ProjectileExplosionRadius(), "manshredder::secondary::projectileExplosionRadius");
    fnFloat(config::ProjectileExplosionForce(), "manshredder::secondary::projectileExplosionForce");
    fnInt(config::ProjectileSplashDamageMin(), "manshredder::secondary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "manshredder::secondary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "manshredder::secondary::projectileDirectDamage");
  }

  {
    namespace config = plugin::config::wallbanger::primary;
    fnInt(config::DischargeCooldown(), "wallbanger::primary::dischargeCooldown");
    fnFloat(config::ProjectileVelocity(), "wallbanger::primary::projectileVelocity");
    fnFloat(config::ProjectileForce(), "wallbanger::primary::projectileForce");
    fnInt(config::ProjectileSplashDamageMin(), "wallbanger::primary::projectileSplashDamageMin");
    fnInt(config::ProjectileSplashDamageMax(), "wallbanger::primary::projectileSplashDamageMax");
    fnInt(config::ProjectileDirectDamage(), "wallbanger::primary::projectileDirectDamage");
  }

  {
    namespace config = plugin::config::wallbanger::secondary;
    fnInt(config::DischargeCooldown(), "wallbanger::secondary::dischargeCooldown");
    fnFloat(config::ProjectileForce(), "wallbanger::secondary::projectileForce");
    fnInt(config::ProjectileDamage(), "wallbanger::secondary::projectileDamage");
  }

  {
    namespace config = plugin::config::weapon;
    fnInt(config::WeaponSwitchCooldown(), "weapon::weaponSwitchCooldown");
  }
}

}

void plugin::config::RenderImGui() {
  ImGui::Begin("weapon config");

  ImGui::PushItemWidth(64.0f);
  ::ApplyToConfig(
    [](float & value, std::string const & label) -> void {
      ImGui::DragFloat(label.c_str(), &value, 0.005f);
    }
  , [](int32_t & value, std::string const & label) -> void {
      pul::imgui::DragInt(label.c_str(), &value, 0.005f);
    }
  , [](std::vector<float> & value, std::string const & label) ->void{
      for (size_t i = 0ul; i < value.size(); ++ i) {
        ImGui::DragFloat(
          (label + "-" + std::to_string(i)).c_str(), &value[i], 0.005f
        );
      }
    }
  );
  ImGui::PopItemWidth();

  ImGui::End();
}

void plugin::config::SaveConfig() {

  cJSON * configJson = cJSON_CreateObject();

  ::ApplyToConfig(
    [&configJson](float & value, std::string const & label) -> void {
      cJSON_AddItemToObject(
        configJson, label.c_str()
      , cJSON_CreateNumber(static_cast<double>(value))
      );
    }
  , [&configJson](int32_t & value, std::string const & label) -> void {
      cJSON_AddItemToObject(configJson, label.c_str(), cJSON_CreateInt(value));
    }
  , [&configJson](std::vector<float> & value, std::string const & label) ->void{
      for (size_t i = 0ul; i < value.size(); ++ i) {
        cJSON_AddItemToObject(
          configJson, (label + "-" + std::to_string(i)).c_str()
        , cJSON_CreateNumber(static_cast<double>(value[i]))
        );
      }
    }
  );

  { // -- save file
    auto jsonStr = cJSON_Print(configJson);

    auto configFilename = "assets/base/config.json";

    spdlog::info("saving json: '{}'", configFilename);

    auto file = std::ofstream{configFilename};
    if (file.good()) {
      file << jsonStr;
    } else {
      spdlog::error("could not save file");
    }
  }
}

void plugin::config::LoadConfig() {
  cJSON * configJson = ::LoadJsonFile("assets/base/config.json");

  ::ApplyToConfig(
    [&configJson](float & value, std::string const & label) -> void {
      value =
        static_cast<float>(
          cJSON_GetObjectItemCaseSensitive(configJson, label.c_str())
            ->valuedouble
        );
    }
  , [&configJson](int32_t & value, std::string const & label) -> void {
      value =
        static_cast<int32_t>(
          cJSON_GetObjectItemCaseSensitive(configJson, label.c_str())
            ->valueint
        );
    }
  , [&configJson](std::vector<float> & value, std::string const & label) ->void{
      for (size_t i = 0ul; i < value.size(); ++ i) {
        value[i] =
          static_cast<float>(
            cJSON_GetObjectItemCaseSensitive(
              configJson
            , (label + "-" + std::to_string(i)).c_str()
            )->valuedouble
          );
      }
    }
  );
}

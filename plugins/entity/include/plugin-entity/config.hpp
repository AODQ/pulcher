#pragma once

#include <vector>

namespace plugin::config {

  namespace volnias::primary {
    int32_t & ChargeupPreBeginThreshold();
    int32_t & ChargeupBeginThreshold();
    int32_t & ChargeupDelta();
    int32_t & ChargeupTimerEnd();
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileForce();
    int32_t & ProjectileDamage();
    float & Knockback();
  }

  namespace volnias::secondary {
    int32_t & MaxChargedShots();
    int32_t & ChargeupDelta();
    int32_t & ChargeupMaxThreshold();
    int32_t & ChargeupTimerStart();
    int32_t & DischargeDelta();

    int32_t & DischargeCooldown();
  }

  namespace grannibal::primary {
    int32_t & MuzzleTrailTimer();
    int32_t & MuzzleTrailParticles();

    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    int32_t & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();
  }

  namespace grannibal::secondary {
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    int32_t & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();

    int32_t & Bounces();
    float & ProjectileVelocityFriction();
  }

  namespace dopplerBeam::primary {
    float & ProjectileVelocity();
    float & ProjectileForce();
    int32_t & ProjectileDamage();
    int32_t & DischargeCooldown();
  }

  namespace dopplerBeam::secondary {
    float & ProjectileVelocity();
    float & ProjectileForce();
    int32_t & ProjectileDamage();
    std::vector<float> & ShotPattern();
    int32_t & DischargeCooldown();
  }

  namespace pericaliya::primary {
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    int32_t & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();
  }

  namespace pericaliya::secondary {
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    int32_t & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();

    int32_t & RedirectionMinimumThreshold();

    std::vector<float> & ShotPattern();
  }

  namespace zeusStinger::primary {
    int32_t & DischargeCooldown();
    float & ProjectileForce();
    int32_t & ProjectileDamage();
  }

  namespace zeusStinger::secondary {
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileVelocityFriction();
    int32_t & ProjectileLifetime();
    int32_t & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();

    int32_t & RedirectionMinimumThreshold();

    std::vector<float> & ShotPattern();
  }

  namespace badFetus::primary {
    int32_t & DischargeCooldown();
    float & ProjectileForce();
    int32_t & ProjectileCooldown();
    int32_t & ProjectileDamage();
  }

  namespace badFetus::secondary {
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileVelocityFriction();
    int32_t & ProjectileLifetime();
    int32_t & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();
  }

  namespace badFetus::combo {
    float & VelocityFriction();
    int32_t & ExplosionRadius();
    float & ExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();
  }

  namespace manshredder::primary {
    int32_t & DischargeCooldown();
    float & ProjectileForce();
    int32_t & ProjectileCooldown();
    int32_t & ProjectileDamage();
    int32_t & ProjectileDistance();
  }

  namespace manshredder::secondary {
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    int32_t & ProjectileExplosionRadius();
    float & ProjectileExplosionForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();
  }

  namespace wallbanger::primary {
    int32_t & DischargeCooldown();
    float & ProjectileVelocity();
    float & ProjectileForce();
    int32_t & ProjectileSplashDamageMin();
    int32_t & ProjectileSplashDamageMax();
    int32_t & ProjectileDirectDamage();
  }

  namespace wallbanger::secondary {
    int32_t & DischargeCooldown();
    float & ProjectileForce();
    int32_t & ProjectileDamage();
  }

  namespace weapon {
    int32_t & WeaponSwitchCooldown();
  }

  void SaveConfig();
  void LoadConfig();

  void RenderImGui();
}

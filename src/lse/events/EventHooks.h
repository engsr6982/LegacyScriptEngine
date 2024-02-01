#pragma once

namespace lse::events {

void ActorRideEvent();
void ArmorStandSwapItemEvent();
void ContainerChangeEvent();
void FarmDecayEvent();
void PistonPushEvent();
void PlayerAttackBlockEvent();
void PlayerChangeSlotEvent();
void PlayerCloseContainerEvent();
void PlayerDropItem();
void PlayerOpenContainerEvent();
void PlayerStartDestroyBlock();
void PlayerUseFrameEvent();
void PressurePlateTriggerEvent();
void ProjectileCreatedEvent();
void ProjectileSpawnEvent();
void WitherDestroyEvent();
void PlayerEatEvent();
void ExplodeEvent();
void RespawnAnchorExplodeEvent();
void BlockExplodedEvent();

} // namespace lse::events

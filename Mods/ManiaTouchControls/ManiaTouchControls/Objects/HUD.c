#include "HUD.h"
#include "Player.h"
#include "../ModConfig.h"

ObjectHUD *HUD;
ModObjectHUD *Mod_HUD;

void (*HUD_DrawNumbersBase16)(Vector2 *drawPos, int32 value) = NULL;

// Hold three fingers for approximately one second to toggle the overlay.
// The input hooks remain active even when the overlay is hidden.
static int32 touchVisibilityHold = 0;
static bool32 touchVisibilityLatched = false;
static int32 touchGestureFingers = 0;
static float touchVisibleOpacity = 0.42f;

// Wait until the gesture has settled, then hold for 45 frames.
// This prevents a four-finger touch from being mistaken for a three-finger
// hide command while the fourth finger is landing.
static void HUD_UpdateTouchVisibility(void)
{
    int32 pressed = 0;
    for (int32 i = 0; i < TouchInfo->count; ++i) {
        if (TouchInfo->down[i])
            ++pressed;
    }

    if (pressed == 0) {
        touchVisibilityHold = 0;
        touchVisibilityLatched = false;
        touchGestureFingers = 0;
        return;
    }
    if (touchVisibilityLatched)
        return;

    if (pressed < 3) {
        touchVisibilityHold = 0;
        touchGestureFingers = 0;
        return;
    }

    int32 gesture = pressed >= 4 ? 4 : 3;
    if (touchGestureFingers != gesture) {
        touchGestureFingers = gesture;
        touchVisibilityHold = 0;
        return;
    }

    if (++touchVisibilityHold < 45)
        return;

    if (gesture == 4) {
        // Use explicit steps to avoid a saved intermediate float value
        // becoming stuck at the maximum setting.
        float current = config.vDPadOpacity > 0.0f ? config.vDPadOpacity : touchVisibleOpacity;
        if (current < 0.375f)
            touchVisibleOpacity = 0.50f;
        else if (current < 0.625f)
            touchVisibleOpacity = 0.75f;
        else if (current < 0.875f)
            touchVisibleOpacity = 1.0f;
        else
            touchVisibleOpacity = 0.25f;
        config.vDPadOpacity = touchVisibleOpacity;
    }
    else if (config.vDPadOpacity > 0.0f) {
        touchVisibleOpacity = config.vDPadOpacity;
        config.vDPadOpacity = 0.0f;
    }
    else {
        config.vDPadOpacity = touchVisibleOpacity;
    }

    Mod.SetSettingsFloat("Config:vDPadOpacity", config.vDPadOpacity);
    Mod.SaveSettings();
    touchVisibilityLatched = true;
}

// Experimental palette-independent touch artwork. The input hitboxes and
// gesture handling remain unchanged. All colors below are true RGB values,
// not indices into the active stage palette.
// Palette-independent, translucent mobile controls. All geometry is visual
// only: input hitboxes, touch processing and gesture logic are unchanged.
static void HUD_DrawRGBTouch(Animator *animator, Vector2 *pos, bool32 screenRelative)
{
    RSDK_THIS(HUD);
    if (!pos || self->alpha <= 0)
        return;

    int32 x = FROM_FIXED(pos->x);
    int32 y = FROM_FIXED(pos->y);
    int32 radius = (int32)(23.0f * config.vDPadSize);
    if (radius < 8) radius = 8;
    int32 a = self->alpha;
    if (a > 255) a = 255;
    const uint32 fill = 0x648CA9;
    const uint32 rim = 0xD6ECFA;
    const uint32 active = 0xB9E5FF;
    const uint32 glyph = 0xE8F6FF;
    int32 frame = animator->frameID;
    bool32 pressed = animator == &Mod_HUD->dpadTouchAnimator;
    int32 fillAlpha = pressed ? (a * 3) / 5 : (a * 2) / 5;
    int32 rimAlpha = (a * 4) / 5;

    if (frame == 10) {
        // A very light outer guide: retain directional clarity without
        // obscuring the stage or competing with the four arrow buttons.
        RSDK.DrawCircle(x, y, radius + 8, fill, a / 14, INK_ALPHA, screenRelative);
        RSDK.DrawCircleOutline(x, y, radius + 7, radius + 8, rim, a / 3, INK_ALPHA, screenRelative);
        RSDK.DrawCircle(x, y, 3, glyph, a / 2, INK_ALPHA, screenRelative);
        return;
    }
    if (frame == 11) {
        RSDK.DrawCircle(x, y, 4, glyph, rimAlpha, INK_ALPHA, screenRelative);
        return;
    }
    if (frame >= 6 && frame <= 9) {
        int32 dx = frame == 6 ? -radius : frame == 7 ? radius : 0;
        int32 dy = frame == 8 ? -radius : frame == 9 ? radius : 0;
        int32 bx = x + dx, by = y + dy;
        int32 pad = radius / 2;
        if (pad < 8) pad = 8;
        RSDK.DrawCircle(bx, by, pad, pressed ? active : fill, fillAlpha, INK_ALPHA, screenRelative);
        RSDK.DrawCircleOutline(bx, by, pad - 2, pad, rim, rimAlpha, INK_ALPHA, screenRelative);
        // Four compact directional chevrons, in true RGB rather than stage indices.
        if (frame == 6) {
            RSDK.DrawLine(bx + 3, by - 4, bx - 2, by, glyph, a, INK_ALPHA, screenRelative);
            RSDK.DrawLine(bx - 2, by, bx + 3, by + 4, glyph, a, INK_ALPHA, screenRelative);
        }
        else if (frame == 7) {
            RSDK.DrawLine(bx - 3, by - 4, bx + 2, by, glyph, a, INK_ALPHA, screenRelative);
            RSDK.DrawLine(bx + 2, by, bx - 3, by + 4, glyph, a, INK_ALPHA, screenRelative);
        }
        else if (frame == 8) {
            RSDK.DrawLine(bx - 4, by + 3, bx, by - 2, glyph, a, INK_ALPHA, screenRelative);
            RSDK.DrawLine(bx, by - 2, bx + 4, by + 3, glyph, a, INK_ALPHA, screenRelative);
        }
        else {
            RSDK.DrawLine(bx - 4, by - 3, bx, by + 2, glyph, a, INK_ALPHA, screenRelative);
            RSDK.DrawLine(bx, by + 2, bx + 4, by - 3, glyph, a, INK_ALPHA, screenRelative);
        }
        return;
    }
    if (frame >= 12 && frame <= 15)
        return;

    int32 small = frame == 5 ? 11 : radius;
    RSDK.DrawCircle(x, y, small, pressed ? active : fill, fillAlpha, INK_ALPHA, screenRelative);
    RSDK.DrawCircleOutline(x, y, small - 2, small, rim, rimAlpha, INK_ALPHA, screenRelative);
    if (frame == 5) {
        RSDK.DrawRect(x - 4, y - 5, 3, 10, glyph, rimAlpha, INK_ALPHA, screenRelative);
        RSDK.DrawRect(x + 2, y - 5, 3, 10, glyph, rimAlpha, INK_ALPHA, screenRelative);
    }
    else {
        RSDK.DrawCircle(x, y, 4, glyph, rimAlpha, INK_ALPHA, screenRelative);
    }
}

void HUD_DrawTouchControls(void)
{
    RSDK_THIS(HUD);

    HUD_UpdateTouchVisibility();

    // Invisible mode must suppress rendering outright: cached fade alphas
    // otherwise keep the buttons visible even when opacity is set to zero.
    if (config.vDPadOpacity <= 0.0f)
        return;

    int32 playerID                  = SceneInfo->currentScreenID;
    EntityPlayer *player            = RSDK_GET_ENTITY(playerID, Player);

    int32 alphaStore   = self->alpha;
    int32 inkStore     = self->inkEffect;
    int32 fxStore      = self->drawFX;
    Vector2 scaleStore = self->scale;

    Mod_HUD->dpadPos.x = TO_FIXED(config.moveDPadPos.x);
    Mod_HUD->dpadPos.y = TO_FIXED(config.moveDPadPos.y);

    Mod_HUD->actionPos.x = TO_FIXED(ScreenInfo[SceneInfo->currentScreenID].size.x + config.jumpDPadPos.x);
    Mod_HUD->actionPos.y = TO_FIXED(config.jumpDPadPos.y);
    
#if GAME_VERSION != VER_100
    Mod_HUD->superPos.x = Mod_HUD->actionPos.x - TO_FIXED(64);
    Mod_HUD->superPos.y = Mod_HUD->actionPos.y;
#endif

#if MANIA_USE_PLUS
    Mod_HUD->swapPos.x = Mod_HUD->actionPos.x;
    Mod_HUD->swapPos.y = Mod_HUD->actionPos.y - TO_FIXED(64);

    if (globals->gameMode == MODE_ENCORE) 
        Mod_HUD->pausePos.x = TO_FIXED(ScreenInfo[SceneInfo->currentScreenID].size.x - 100);
    else 
#endif
        Mod_HUD->pausePos.x = TO_FIXED(ScreenInfo[SceneInfo->currentScreenID].size.x - 76);
    Mod_HUD->pausePos.y = TO_FIXED(16);

    self->inkEffect = INK_ALPHA;
    self->drawFX    = FX_SCALE;

    int32 opacity = (int32)(0x100 * config.vDPadOpacity);
    // Keep fade caches within the newly selected opacity immediately.
    if (Mod_HUD->dpadAlpha[playerID] > opacity) Mod_HUD->dpadAlpha[playerID] = opacity;
    if (Mod_HUD->jumpAlpha[playerID] > opacity) Mod_HUD->jumpAlpha[playerID] = opacity;
#if GAME_VERSION != VER_100
    if (Mod_HUD->superAlpha[playerID] > opacity) Mod_HUD->superAlpha[playerID] = opacity;
#endif
#if MANIA_USE_PLUS
    if (Mod_HUD->swapAlpha[playerID] > opacity) Mod_HUD->swapAlpha[playerID] = opacity;
#endif
    if (Mod_HUD->pauseAlpha[playerID] > opacity) Mod_HUD->pauseAlpha[playerID] = opacity;
    self->scale.x = (int32)(0x200 * config.vDPadSize);
    self->scale.y = (int32)(0x200 * config.vDPadSize);

    bool32 canMove = player->stateInput == Player_Input_P1;
    canMove |= player->stateInput == MegaChopper_Input_GrabbedP1;
    canMove |= player->stateInput == Gachapandora_Player_StateInput_P1Grabbed;
#if MANIA_USE_PLUS
    canMove |= player->stateInput == EncoreIntro_PlayerInput_BuddySel;
#endif

    bool32 canJump = player->stateInput == Player_Input_P1;
    canJump |= player->stateInput == MegaChopper_Input_GrabbedP1;
    canJump |= player->stateInput == Gachapandora_Player_StateInput_P1Grabbed;

#if GAME_VERSION != VER_100
    bool32 canSuper = canJump && Player_CanTransform(player);
#endif
#if MANIA_USE_PLUS
    bool32 canSwap = canJump && globals->gameMode == MODE_ENCORE && !HUD->swapCooldown && Player_CheckValidState(player) && Player_CanSwap(player);
#endif

    bool32 canPause = canMove;

#if GAME_VERSION != VER_100
    Vector2 superPos = Mod_HUD->superPos;
#endif
#if MANIA_USE_PLUS
    Vector2 swapPos  = Mod_HUD->swapPos;
#endif

    RSDKControllerState *controller = &ControllerInfo[player->controllerID];

    if (canMove) {
        if (Mod_HUD->dpadAlpha[playerID] < opacity) 
            Mod_HUD->dpadAlpha[playerID] += 4;

        // Draw DPad
        self->alpha                   = Mod_HUD->dpadAlpha[playerID];
        Mod_HUD->dpadAnimator.frameID = 10;
        HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->dpadPos, true);

        if (player->classID == Player->classID ? player->left : controller->keyLeft.down) {
            self->alpha                        = opacity;
            Mod_HUD->dpadTouchAnimator.frameID = 6;
            HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);
        }
        else {
            self->alpha                   = Mod_HUD->dpadAlpha[playerID];
            Mod_HUD->dpadAnimator.frameID = 6;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->dpadPos, true);
        }

        if (player->classID == Player->classID ? player->down : controller->keyDown.down) {
            self->alpha                        = opacity;
            Mod_HUD->dpadTouchAnimator.frameID = 9;
            HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);

            if (player->classID == Player->classID ? player->left : controller->keyLeft.down) {
                Mod_HUD->dpadTouchAnimator.frameID = 14;
                HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);
            }
            else if (player->classID == Player->classID ? player->right : controller->keyRight.down) {
                Mod_HUD->dpadTouchAnimator.frameID = 15;
                HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);
            }
        }
        else {
            self->alpha                   = Mod_HUD->dpadAlpha[playerID];
            Mod_HUD->dpadAnimator.frameID = 9;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->dpadPos, true);
        }

        if (player->classID == Player->classID ? player->right : controller->keyRight.down) {
            self->alpha                        = opacity;
            Mod_HUD->dpadTouchAnimator.frameID = 7;
            HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);
        }
        else {
            self->alpha                   = Mod_HUD->dpadAlpha[playerID];
            Mod_HUD->dpadAnimator.frameID = 7;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->dpadPos, true);
        }

        if (player->classID == Player->classID ? player->up : controller->keyUp.down) {
            self->alpha                        = opacity;
            Mod_HUD->dpadTouchAnimator.frameID = 8;
            HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);

            if (player->classID == Player->classID ? player->left : controller->keyLeft.down) {
                Mod_HUD->dpadTouchAnimator.frameID = 12;
                HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);
            }
            else if (player->classID == Player->classID ? player->right : controller->keyRight.down) {
                Mod_HUD->dpadTouchAnimator.frameID = 13;
                HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->dpadPos, true);
            }
        }
        else {
            self->alpha                   = Mod_HUD->dpadAlpha[playerID];
            Mod_HUD->dpadAnimator.frameID = 8;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->dpadPos, true);
        }

        if (player->classID == Player->classID ? (!player->up && !player->down && !player->left && !player->right) : (!controller->keyUp.down && !controller->keyDown.down && !controller->keyLeft.down && !controller->keyRight.down)) {
            self->alpha                   = Mod_HUD->dpadAlpha[playerID];
            Mod_HUD->dpadAnimator.frameID = 11;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->dpadPos, true);
        }
    }
    else {
        if (Mod_HUD->dpadAlpha[playerID] > 0) {
            Mod_HUD->dpadAlpha[playerID] -= 4;
        }

        self->alpha = Mod_HUD->dpadAlpha[playerID];
        if (self->alpha > 0) {
            Mod_HUD->dpadAnimator.frameID = 0;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->dpadPos, true);
        }
    }

    if (canJump) {
        if ((SceneInfo->state & 3) == ENGINESTATE_REGULAR) {
            if (Mod_HUD->jumpAlpha[playerID] < opacity)
                Mod_HUD->jumpAlpha[playerID] += 4;

            if (player->classID == Player->classID ? player->jumpHold : controller->keyC.down) {
                self->alpha                        = opacity;
                Mod_HUD->dpadTouchAnimator.frameID = 1;
                HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->actionPos, true);
            }
            else {
                self->alpha                   = Mod_HUD->jumpAlpha[playerID];
                Mod_HUD->dpadAnimator.frameID = 1;
                HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->actionPos, true);
            }
        }
        else {
            Mod_HUD->jumpAlpha[playerID] = 0;
        }
    }
    else {
        if (Mod_HUD->jumpAlpha[playerID] > 0) 
            Mod_HUD->jumpAlpha[playerID] -= 4;

        self->alpha = Mod_HUD->jumpAlpha[playerID];
        if (self->alpha > 0) {
            Mod_HUD->dpadAnimator.frameID = 1;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &Mod_HUD->actionPos, true);
        }
    }

#if GAME_VERSION != VER_100
    if (canSuper) {
        if ((SceneInfo->state & 3) == ENGINESTATE_REGULAR) {
            if (Mod_HUD->superAlpha[playerID] < opacity)
                Mod_HUD->superAlpha[playerID] += 4;

            if (!player->onGround && ControllerInfo[player->controllerID].keyY.down) {
                self->alpha                        = opacity;
                Mod_HUD->dpadTouchAnimator.frameID = 3;
                HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &superPos, true);
            }
            else {
                self->alpha                   = Mod_HUD->superAlpha[playerID];
                Mod_HUD->dpadAnimator.frameID = 3;
                HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &superPos, true);
            }
        }
        else {
            Mod_HUD->superAlpha[playerID] = 0;
        }
    }
    else {
        if (Mod_HUD->superAlpha[playerID] > 0)
            Mod_HUD->superAlpha[playerID] -= 4;

        self->alpha = Mod_HUD->superAlpha[playerID];
        if (self->alpha > 0) {
            Mod_HUD->dpadAnimator.frameID = 3;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &superPos, true);
        }
    }
#endif

#if MANIA_USE_PLUS
    if (canSwap) {
        if ((SceneInfo->state & 3) == ENGINESTATE_REGULAR) {
            if (Mod_HUD->swapAlpha[playerID] < opacity)
                Mod_HUD->swapAlpha[playerID] += 4;

            if (player->onGround && ControllerInfo[player->controllerID].keyY.down) {
                self->alpha                        = opacity;
                Mod_HUD->dpadTouchAnimator.frameID = 4;
                HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &swapPos, true);
            }
            else {
                self->alpha                   = Mod_HUD->swapAlpha[playerID];
                Mod_HUD->dpadAnimator.frameID = 4;
                HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &swapPos, true);
            }
        }
        else {
            Mod_HUD->swapAlpha[playerID] = 0;
        }
    }
    else {
        if (Mod_HUD->swapAlpha[playerID] > 0)
            Mod_HUD->swapAlpha[playerID] -= 4;

        self->alpha = Mod_HUD->swapAlpha[playerID];
        if (self->alpha > 0) {
            Mod_HUD->dpadAnimator.frameID = 4;
            HUD_DrawRGBTouch(&Mod_HUD->dpadAnimator, &swapPos, true);
        }
    }
#endif

    if (canPause) {
        if ((SceneInfo->state & 3) == ENGINESTATE_REGULAR) {
            if (Mod_HUD->pauseAlpha[playerID] < opacity)
                Mod_HUD->pauseAlpha[playerID] += 8;

            self->alpha                        = Mod_HUD->pauseAlpha[playerID];
            Mod_HUD->dpadTouchAnimator.frameID = 5;
            HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->pausePos, true);
        }
        else {
            Mod_HUD->pauseAlpha[playerID] = 0;
        }
    }
    else {
        if (Mod_HUD->pauseAlpha[playerID] > 0)
            Mod_HUD->pauseAlpha[playerID] -= 8;

        self->alpha = Mod_HUD->pauseAlpha[playerID];
        if (self->alpha > 0) {
            Mod_HUD->dpadTouchAnimator.frameID = 5;
            HUD_DrawRGBTouch(&Mod_HUD->dpadTouchAnimator, &Mod_HUD->pausePos, true);
        }
    }

    self->alpha     = alphaStore;
    self->inkEffect = inkStore;
    self->drawFX    = fxStore;
    self->scale     = scaleStore;
}

void HUD_DrawMobileHUD(void) {
    RSDK_THIS(HUD);
    
    EntityPlayer *player = RSDK_GET_ENTITY(SceneInfo->currentScreenID, Player);
    Vector2 drawPos;

    Object *debugModeSVars = Mod.FindObject("DebugMode");
    if (debugModeSVars && RSDK_GET_ENTITY(SLOT_PLAYER1, Player)->classID == debugModeSVars->classID) {
        if (player->camera) {
            // Draw Camera YPos
            drawPos.x = TO_FIXED(ScreenInfo[player->camera->screenID].size.x - 16);
            drawPos.y = TO_FIXED(42);
            HUD_DrawNumbersBase16(&drawPos, ScreenInfo[player->camera->screenID].position.y);

            // Draw Camera XPos
            drawPos.x -= TO_FIXED(9);
            HUD_DrawNumbersBase16(&drawPos, ScreenInfo[player->camera->screenID].position.x);

            // Draw Player YPos
            drawPos.x = TO_FIXED(ScreenInfo[player->camera->screenID].size.x - 16);
            drawPos.y += TO_FIXED(16);
            HUD_DrawNumbersBase16(&drawPos, FROM_FIXED(player->position.y));

            // Draw Player XPos
            drawPos.x -= TO_FIXED(9);
            HUD_DrawNumbersBase16(&drawPos, FROM_FIXED(player->position.x));
        }
    }
#if MANIA_USE_PLUS
    else if (self->actionPromptPos > -TO_FIXED(64) && globals->gameMode == MODE_TIMEATTACK) {
        drawPos.x = TO_FIXED(ScreenInfo[SceneInfo->currentScreenID].size.x) - self->actionPromptPos;
        drawPos.y = TO_FIXED(48);

        if (API.CheckDLC(DLC_PLUS)) {
            // Draw Replay Save Icon
            RSDK.DrawSprite(&self->replayClapAnimator, &drawPos, true);
            drawPos.y += TO_FIXED(28);
        }
        // Draw Thumbs Up Icon
        RSDK.DrawSprite(&self->thumbsUpIconAnimator, &drawPos, true);
    }
#endif
}

void HUD_Draw(void)
{
    RSDK_THIS(HUD);

    EntityPlayer *player = RSDK_GET_ENTITY(SceneInfo->currentScreenID, Player);

    int32 playerClassID = player->classID;
    void *playerCamera  = player->camera;

    // make the game think we should have debug mode enabled
    // however NULL the camera to draw nothing
    Object *debugModeSVars = Mod.FindObject("DebugMode");
    if (debugModeSVars)
        player->classID = debugModeSVars->classID;
    else
        player->classID = TYPE_BLANK;
    player->camera = NULL;

    Vector2 lifePos = self->lifePos;
#if MANIA_USE_PLUS
    if (globals->gameMode == MODE_COMPETITION) {
        lifePos         = self->vsLifePos[SceneInfo->currentScreenID];

        self->vsLifePos[SceneInfo->currentScreenID].x = TO_FIXED(ScreenInfo[SceneInfo->currentScreenID].size.x - 48) - lifePos.x;
        self->vsLifePos[SceneInfo->currentScreenID].y = TO_FIXED(25);
    }
    else if (globals->gameMode == MODE_ENCORE) {
        self->lifePos.x = TO_FIXED(ScreenInfo[SceneInfo->currentScreenID].size.x - 72) - lifePos.x;
        self->lifePos.y = TO_FIXED(25);
    }
    else {
#endif
        self->lifePos.x = TO_FIXED(ScreenInfo[SceneInfo->currentScreenID].size.x - 48) - lifePos.x;
        self->lifePos.y = TO_FIXED(25);
#if MANIA_USE_PLUS
    }
#endif

    // Draw HUD stuff
    Mod.Super(HUD->classID, SUPER_DRAW, NULL);

    player->classID = playerClassID;
    player->camera  = playerCamera;

    // Draw Mobile HUD bits
    HUD_DrawMobileHUD();

#if MANIA_USE_PLUS
    if (globals->gameMode == MODE_COMPETITION) 
        self->vsLifePos[SceneInfo->currentScreenID] = lifePos;
    else
#endif
        self->lifePos = lifePos;

    HUD_DrawTouchControls();
}

void HUD_StageLoad(void)
{
    Mod.Super(HUD->classID, SUPER_STAGELOAD, NULL);

    Mod_HUD->dpadFrames = RSDK.LoadSpriteAnimation("Global/TouchControls.bin", SCOPE_STAGE);

    RSDK.SetSpriteAnimation(Mod_HUD->dpadFrames, 0, &Mod_HUD->dpadAnimator, true, 0);
    RSDK.SetSpriteAnimation(Mod_HUD->dpadFrames, 1, &Mod_HUD->dpadTouchAnimator, true, 0);
}

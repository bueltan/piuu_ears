#pragma once

bool audioEngineBegin();
// Cooperative frame-boundary pause; failure leaves normal audio running.
bool audioEnginePause();
void audioEngineResume();

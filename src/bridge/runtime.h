#pragma once
void bridgeBegin();
void bridgePoll();
// Implemented by the normal application; called only after EXIT ACK is drained.
void maintenanceResume();

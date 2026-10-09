#pragma once

#ifdef REVC_RENDER_TESTS
void RenderTestInit(void);
void RenderTestFrame(RwCamera *camera);
#else
inline void RenderTestInit(void) {}
inline void RenderTestFrame(RwCamera *) {}
#endif

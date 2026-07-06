#ifndef CULLING_H
#define CULLING_H

#include "raylib.h"
#include "raymath.h"

inline bool EsVisibleEnFrustum(Vector3 punto, const Camera3D& cam,
                                float mitadFovRad, float margenRad = 0.20f,
                                float radioSiempreVisible = 25.0f) {
    Vector3 aCam = Vector3Subtract(punto, cam.position);

    aCam.y = 0.0f;

    float distSq = aCam.x * aCam.x + aCam.z * aCam.z;
    if (distSq <= radioSiempreVisible * radioSiempreVisible) return true;

    Vector3 forward = Vector3Subtract(cam.target, cam.position);
    forward.y = 0.0f;

    float distLen = sqrtf(distSq);
    float fwdLen  = sqrtf(forward.x * forward.x + forward.z * forward.z);
    if (distLen < 0.001f || fwdLen < 0.001f) return true; 

    float cosAngulo = (aCam.x * forward.x + aCam.z * forward.z) / (distLen * fwdLen);
    cosAngulo = Clamp(cosAngulo, -1.0f, 1.0f);
    float angulo = acosf(cosAngulo);

    return angulo <= (mitadFovRad + margenRad);
}

#endif

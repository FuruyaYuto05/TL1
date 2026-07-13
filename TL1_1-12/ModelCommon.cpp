#include "ModelCommon.h"
#include "DirectXCommon.h"

void ModelCommon::Initialize(DirectXCommon* dxCommon)
{
    // 引数で受け取ってメンバ変数に記録する
    dxCommon_ = dxCommon;
}
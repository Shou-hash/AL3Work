#include "Obj.hlsli"

Texture2D<float4> tex : register(t0); // 0番スロットに設定されたテクスチャ
SamplerState smp : register(s0); // 0番スロットに設定されたサンプラー

// 拡散反射光（Lambert / Half Lambert）の計算関数
float3 CalculateDiffuse(float NdotL, float3 materialDiffuse, uint type)
{
    if (type == 1)
    {
		// Half Lambert: 影の部分を明るく柔らかく表現 ( -1~1 -> 0~1 に補正して2乗 )
        float halfLambert = NdotL * 0.5f + 0.5f;
        return halfLambert * halfLambert * materialDiffuse;
    }
    else
    {
		// Standard Lambert: 影をくっきり表現 ( 0.0 ~ 1.0 にクランプ )
        return saturate(NdotL) * materialDiffuse;
    }
}

// 鏡面反射光（Blinn-Phong）の計算関数
// 光が当たっていない面（NdotL <= 0）にはハイライトを出さない
float3 CalculateSpecular(float3 N, float3 halfVector, float NdotL, float shininess, float3 specColor)
{
    return pow(saturate(dot(N, halfVector)), shininess) * saturate(NdotL) * specColor;
}

float4 main(VSOutput input) : SV_TARGET
{
	// UV変換
    float2 uv = float2(
	    input.uv.x * m_uv_scale.x + m_uv_offset.x, input.uv.y * m_uv_scale.y + m_uv_offset.y);
	// テクスチャマッピング
    float4 texcolor = tex.Sample(smp, uv);

	// 法線の正規化
    float3 N = normalize(input.normal);

	// ★ シェーディング種別（LightManager が dirLights[2].color.x に 0:Lambert / 1:HalfLambert を格納している）
    uint lightType = (dirLights[2].color.x > 0.5f) ? 1u : 0u;

	// ★ 光沢の設定（LightManager が dirLights[2].color.y / z に格納している）
    float glossStrength = dirLights[2].color.y; // 光沢の強さ
    float shininess = max(dirLights[2].color.z, 1.0f); // 光沢の鋭さ（大きいほどハイライトが小さく鋭い）
    // 全モデル共通の光沢。ImGui の Gloss Strength を直接反映させるため、マテリアルのスペキュラー係数は使わない
    float3 specColor = float3(glossStrength, glossStrength, glossStrength);
	// 頂点から視点への方向ベクトル
    float3 viewDir = normalize(cameraPos - input.worldpos.xyz);

	// 環境反射光
    float3 ambient = m_ambient;

	// シェーディングによる色初期値
    float4 shadecolor = float4(ambientColor * ambient, m_alpha);

	// 鏡面反射光の合計（テクスチャの色を掛けずに、光の色のままハイライトとして加算する）
    float3 specularSum = float3(0.0f, 0.0f, 0.0f);

	// 平行光源
    for (int i = 0; i < DIRLIGHT_NUM; i++)
    {
        if (dirLights[i].active)
        {
            float3 L = normalize(dirLights[i].direction);
            float NdotL = dot(L, N);

			// ハーフベクトル
            float3 halfVector = normalize(L + viewDir);

			// ★ Lambert / Half Lambert 拡散反射光
            float3 diffuse = CalculateDiffuse(NdotL, m_diffuse, lightType);
			// 鏡面反射光 (Blinn-Phong)
            float3 specular = CalculateSpecular(N, halfVector, NdotL, shininess, specColor);

			// 拡散反射光は通常通り、鏡面反射光は別に加算する
            shadecolor.rgb += diffuse * dirLights[i].color;
            specularSum += specular * dirLights[i].color;
        }
    }

	// 点光源
    for (i = 0; i < POINTLIGHT_NUM; i++)
    {
        if (pointLights[i].active)
        {
            float3 direction = pointLights[i].position - input.worldpos.xyz;
            float distance = length(direction);
            direction = normalize(direction);

            float NdotL = dot(direction, N);
            float3 halfVector = normalize(direction + viewDir);

            float factor = pow(saturate(1.0f - distance / pointLights[i].radius), pointLights[i].decay);

			// ★ Lambert / Half Lambert 拡散反射光
            float3 diffuse = CalculateDiffuse(NdotL, m_diffuse, lightType);
            float3 specular = CalculateSpecular(N, halfVector, NdotL, shininess, specColor);

            shadecolor.rgb += pointLights[i].color * pointLights[i].intensity * diffuse * factor;
            specularSum += pointLights[i].color * pointLights[i].intensity * specular * factor;
        }
    }

	// スポットライト
    for (i = 0; i < SPOTLIGHT_NUM; i++)
    {
        if (spotLights[i].active)
        {
            float3 direction = spotLights[i].position - input.worldpos.xyz;
            float distance = length(direction);
            direction = normalize(direction);

            float distanceFactor = pow(saturate(1.0f - distance / spotLights[i].radius), spotLights[i].decay);
            float cosTheta = dot(-direction, spotLights[i].direction);
            float angleFactor = smoothstep(spotLights[i].cosAngle.y, spotLights[i].cosAngle.x, cosTheta);
            float factor = distanceFactor * angleFactor;

            float NdotL = dot(direction, N);
            float3 halfVector = normalize(direction + viewDir);

			// ★ Lambert / Half Lambert 拡散反射光
            float3 diffuse = CalculateDiffuse(NdotL, m_diffuse, lightType);
            float3 specular = CalculateSpecular(N, halfVector, NdotL, shininess, specColor);

            shadecolor.rgb += spotLights[i].color * spotLights[i].intensity * diffuse * factor;
            specularSum += spotLights[i].color * spotLights[i].intensity * specular * factor;
        }
    }

	// 丸影
    for (i = 0; i < CIRCLESHADOW_NUM; i++)
    {
        if (circleShadows[i].active)
        {
            float3 direction = circleShadows[i].position - input.worldpos.xyz;
            float distance = dot(direction, circleShadows[i].direction);

            float distanceFactor = saturate(
			    1.0f / (circleShadows[i].atten.x + circleShadows[i].atten.y * distance +
			            circleShadows[i].atten.z * distance * distance));
            distanceFactor *= step(0, distance);

            float3 lightPos = circleShadows[i].position +
			                  circleShadows[i].direction * circleShadows[i].distanceCasterLight;
            float3 L = normalize(lightPos - input.worldpos.xyz);
            float cosTheta = dot(L, circleShadows[i].direction);
            float angleFactor = smoothstep(circleShadows[i].cosAngle.y, circleShadows[i].cosAngle.x, cosTheta);

            float factor = distanceFactor * angleFactor;
            shadecolor.rgb -= factor;
        }
    }

	// シェーディングによる色で描画（鏡面反射光はテクスチャの色を掛けずに加算して、光沢感を出す）
    float4 result = shadecolor * texcolor * color;
    result.rgb += specularSum;

	// ★ 自己発光（ラスボス専用。LightManager が pointLights[2].intensity に格納している。他のモデルは 0）
	// スポットライトのように光の色でモデルを染めず、モデル本来の色を保ったまま明るく光らせる。輪郭ほど強く光る
    float glow = pointLights[2].intensity;
    float3 baseColor = texcolor.rgb * color.rgb;
    float rim = pow(1.0f - saturate(dot(N, viewDir)), 2.0f);
    result.rgb += baseColor * glow * (0.6f + 1.4f * rim);
    return result;
}
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

float4 main(VSOutput input) : SV_TARGET
{
	// UV変換
    float2 uv = float2(
	    input.uv.x * m_uv_scale.x + m_uv_offset.x, input.uv.y * m_uv_scale.y + m_uv_offset.y);
	// テクスチャマッピング
    float4 texcolor = tex.Sample(smp, uv);

	// 法線の正規化
    float3 N = normalize(input.normal);

	// 光沢度
    const float shininess = 4.0f;
	// 頂点から視点への方向ベクトル
    float3 viewDir = normalize(cameraPos - input.worldpos.xyz);

	// 環境反射光
    float3 ambient = m_ambient;

	// シェーディングによる色初期値
    float4 shadecolor = float4(ambientColor * ambient, m_alpha);

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
            float3 specular = pow(saturate(dot(N, halfVector)), shininess) * m_specular;

			// 全て加算する
            shadecolor.rgb += (diffuse + specular) * dirLights[i].color;
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
            float3 specular = pow(saturate(dot(N, halfVector)), shininess) * m_specular;

            shadecolor.rgb += pointLights[i].color * pointLights[i].intensity * (diffuse + specular) * factor;
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
            float cosTheta = dot(direction, spotLights[i].direction);
            float angleFactor = smoothstep(spotLights[i].cosAngle.y, spotLights[i].cosAngle.x, cosTheta);
            float factor = distanceFactor * angleFactor;

            float NdotL = dot(direction, N);
            float3 halfVector = normalize(direction + viewDir);

			// ★ Lambert / Half Lambert 拡散反射光
            float3 diffuse = CalculateDiffuse(NdotL, m_diffuse, lightType);
            float3 specular = pow(saturate(dot(N, halfVector)), shininess) * m_specular;

            shadecolor.rgb += spotLights[i].color * spotLights[i].intensity * (diffuse + specular) * factor;
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

	// シェーディングによる色で描画
    return shadecolor * texcolor * color;
}
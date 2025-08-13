// Private/Types/EconomyTypes.cpp

#include "Types/EconomyTypes.h"

EMarketDemand FMarketData::CalculateDemandLevel() const
{
	if (Supply <= 0 && Demand > 0)
	{
		return EMarketDemand::Critical;
	}

	if (Supply <= 0)
	{
		return EMarketDemand::VeryHigh;
	}

	float Ratio = static_cast<float>(Demand) / static_cast<float>(Supply);

	if (Ratio >= 3.0f)
	{
		return EMarketDemand::VeryHigh;
	}
	else if (Ratio >= 1.5f)
	{
		return EMarketDemand::High;
	}
	else if (Ratio >= 0.5f)
	{
		return EMarketDemand::Normal;
	}
	else if (Ratio >= 0.2f)
	{
		return EMarketDemand::Low;
	}
	else
	{
		return EMarketDemand::VeryLow;
	}
}

float FMarketData::GetPriceModifier() const
{
	if (Supply <= 0 && Demand > 0)
	{
		return 3.0f; // Critical shortage
	}

	if (Supply <= 0)
	{
		return 2.0f; // No supply
	}

	float Ratio = static_cast<float>(Demand) / static_cast<float>(Supply);

	// Price increases with demand/supply ratio
	if (Ratio >= 3.0f)
	{
		return 2.5f; // Very high demand
	}
	else if (Ratio >= 1.5f)
	{
		return 1.5f; // High demand
	}
	else if (Ratio >= 0.5f)
	{
		return 1.0f; // Normal balance
	}
	else if (Ratio >= 0.2f)
	{
		return 0.75f; // Low demand
	}
	else
	{
		return 0.5f; // Very low demand/surplus
	}
}
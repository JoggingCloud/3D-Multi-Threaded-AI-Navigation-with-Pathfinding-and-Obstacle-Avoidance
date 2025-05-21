#include "Game/ActorUID.hpp"

const ActorUID ActorUID::INVALID = ActorUID(0xFFFF, 0xFFFF);

ActorUID::ActorUID()
	: m_data(INVALID.m_data)
{
}

ActorUID::ActorUID(unsigned int salt, unsigned int index)
{
	m_data = (salt << 16) | (index & 0xFFFF);
}

bool ActorUID::IsValid() const
{
	return m_data != INVALID.m_data;
}

unsigned int ActorUID::GetIndex() const
{
	return m_data & 0xFFFF;
}

bool ActorUID::operator==(const ActorUID& other) const
{
	return m_data == other.m_data;
}

bool ActorUID::operator!=(const ActorUID& other) const
{
	return m_data != other.m_data;
}
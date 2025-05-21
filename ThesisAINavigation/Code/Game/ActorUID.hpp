#pragma once

struct ActorUID
{
public:
	ActorUID();
	ActorUID(unsigned int salt, unsigned int index);

	bool IsValid() const;
	unsigned int GetIndex() const;
	bool operator==(const ActorUID& other) const;
	bool operator!=(const ActorUID& other) const;

	static const ActorUID INVALID;

private:
	unsigned int m_data;
};
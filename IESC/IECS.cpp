#include "IECS.h"

namespace IECS
{
	IWorld::IWorld()
	{}

	IWorld::~IWorld()
	{}

	std::shared_ptr<IWorld> IWorld::CreateWorld()
	{
		return std::shared_ptr<IWorld>(new IWorld);
	}
}


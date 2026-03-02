#pragma once

#include <memory>
#include <vector>
#include <stack>
#include <unordered_map>
#include <cstdint>
#include <typeindex>
#include <utility>
#include <mutex>

namespace IECS
{
	struct IEntity
	{
		size_t Id;
		size_t Version;

		IEntity() : Id(size_t(-1)), Version(size_t(-1)) {}
		IEntity(size_t id, size_t version) : Id(id), Version(version) {}
		IEntity(const IEntity&) = default;
		IEntity(IEntity&&) = default;

		operator bool()
		{
			return Id != size_t(-1) && Version != size_t(-1);
		}

		bool IsValid() const
		{
			return Id != size_t(-1) && Version != size_t(-1);
		}

		bool operator==(const IEntity& other) const
		{
			return Id == other.Id && Version == other.Version;
		}

		bool operator!=(const IEntity& other) const
		{
			return !(*this == other);
		}

		bool operator==(const std::pair<size_t, size_t>& other) const
		{
			return Id == other.first && Version == other.second;
		}

		bool operator!=(const std::pair<size_t, size_t>& other) const
		{
			return !(*this == other);
		}

		void operator=(const IEntity& other)
		{
			this->Id = other.Id;
			this->Version = other.Version;
		}

		void operator=(const std::pair<size_t, size_t>& other)
		{
			this->Id = other.first;
			this->Version = other.second;
		}
	};

	class IComponentSetBase
	{
	public:
		virtual ~IComponentSetBase() = default;
	};

	template<typename ComponentType>
	class IComponentSet : public IComponentSetBase
	{
		public:
			IComponentSet()
			{
				_components.reserve((size_t)1 << 7);
				_component_indexes.reserve((size_t)1 << 7);
			}
			virtual ~IComponentSet()
			{
				
			}

			template<typename... Args>
			ComponentType* AddComponent(IEntity& entity, Args&&... args)
			{
				if (entity)
				{
					std::lock_guard<std::mutex> lock(_mutex);
					if (entity.Id < _component_indexes.size())
					{
						if (entity.Version == _component_indexes[entity.Id].second)
						{
							// already has component, update it
							_components[_component_indexes[entity.Id].first].first = ComponentType(std::forward<Args>(args)...);
							return &(_components[_component_indexes[entity.Id].first].first);
						}
						else
						{
							// entity version mismatch, update it
							_component_indexes[entity.Id].second = entity.Version;
							_components[_component_indexes[entity.Id].first].first = ComponentType(std::forward<Args>(args)...);
							return &(_components[_component_indexes[entity.Id].first].first);
						}
					}
					else
					{
						// new entity, add it
						size_t index = _components.size();
						_components.emplace_back(ComponentType(std::forward<Args>(args)...), entity);
						if (entity.Id >= _component_indexes.size())
						{
							_component_indexes.resize(entity.Id + 1, std::make_pair(size_t(-1), size_t(-1)));
						}
						_component_indexes[entity.Id] = std::make_pair(index, entity.Version);
						return &(_components[index].first);
					}
				}
				return nullptr;
			}

			bool HasComponent(IEntity& entity)
			{
				if (entity)
				{
					std::lock_guard<std::mutex> lock(_mutex);
					if (entity.Id < _component_indexes.size())
					{
						if (entity.Version == _component_indexes[entity.Id].second)
						{
							return true;
						}
					}
				}
				return false;
			}

			ComponentType* GetComponent(IEntity& entity)
			{
				if (entity)
				{
					if (entity.Id < _component_indexes.size())
					{
						if (entity.Version == _component_indexes[entity.Id].second)
						{
							std::lock_guard<std::mutex> lock(_mutex);

							size_t index = _component_indexes[entity.Id].first;
							if (index < _components.size())
							{
								return &(_components[index].first);
							}
						}
					}
				}
				return nullptr;
			}

			void RemoveComponent(IEntity& entity)
			{
				if (entity)
				{
					if (entity.Id < _component_indexes.size())
					{
						if (entity.Version == _component_indexes[entity.Id].second)
						{

							std::lock_guard<std::mutex> lock(_mutex);

							size_t index = _component_indexes[entity.Id].first;
							size_t last_index = _components.size() - 1;

							if (index == size_t(-1))
							{
								// index is invalid
								return;
							}

							if (index < last_index)
							{
								_components[index] = _components[last_index];
								_component_indexes[_components[index].second.Id] = std::make_pair(index, _components[index].second.Version);
							}
							_components.pop_back();
							_component_indexes[entity.Id] = std::make_pair(size_t(-1), size_t(-1));

							entity = { size_t(-1), size_t(-1) };
						}
					}
				}
			}

			std::vector<std::pair<ComponentType, IEntity>>::iterator begin()
			{
				return _components.begin();
			}
			std::vector<std::pair<ComponentType, IEntity>>::iterator end()
			{
				return _components.end();
			}
			std::vector<std::pair<ComponentType, IEntity>>::reverse_iterator rbegin()
			{
				return _components.rbegin();
			}
			std::vector<std::pair<ComponentType, IEntity>>::reverse_iterator rend()
			{
				return _components.rend();
			}

	private:
		std::vector<std::pair<ComponentType, IEntity>> _components;
		// first: index in _components, second: _components version
		std::vector<std::pair<size_t, size_t>> _component_indexes;

		std::mutex _mutex;
	};

	class IWorld 
	{
		IWorld();
	public:
		virtual ~IWorld();

		static std::shared_ptr<IWorld> CreateWorld();

		template<typename ComponentType, typename... Args>
		ComponentType* AddComponent(IEntity& entity, Args&&... args)
		{
			std::lock_guard<std::mutex> lock(_mutex);

			auto it = _component_sets.find(std::type_index(typeid(ComponentType)));
			if (it != _component_sets.end())
			{
				auto component_set = static_cast<IComponentSet<ComponentType>*>(it->second.get());
				return component_set->AddComponent(entity, std::forward<Args>(args)...);
			}
			else
			{
				auto component_set = std::make_unique<IComponentSet<ComponentType>>();
				auto ref = component_set->AddComponent(entity, std::forward<Args>(args)...);
				_component_sets[std::type_index(typeid(ComponentType))] = std::move(component_set);
				return ref;
			}
		}

		template<typename ComponentType>
		bool HasComponentSet()
		{
			auto it = _component_sets.find(std::type_index(typeid(ComponentType)));
			if (it != _component_sets.end())
			{
				return true;
			}
			return false;
		}

		//template<typename... ComponentTypes>
		//bool HasComponentSet()
		//{
		//	return (HasComponentSet<ComponentTypes>() && ...);
		//}

		template<typename ComponentType>
		bool HasComponent(IEntity& entity)
		{
			auto it = _component_sets.find(std::type_index(typeid(ComponentType)));
			if (it != _component_sets.end())
			{
				auto component_set = static_cast<IComponentSet<ComponentType>*>(it->second.get());
				return component_set->HasComponent(entity);
			}
			return false;
		}

		template<typename ComponentType>
		ComponentType* GetComponent(IEntity& entity)
		{
			auto it = _component_sets.find(std::type_index(typeid(ComponentType)));
			if (it != _component_sets.end())
			{
				auto component_set = static_cast<IComponentSet<ComponentType>*>(it->second.get());
				return component_set->GetComponent(entity);
			}
			return nullptr;
		}

		template<typename ComponentType>
		void RemoveComponent(IEntity& entity)
		{
			auto it = _component_sets.find(std::type_index(typeid(ComponentType)));
			if (it != _component_sets.end())
			{
				auto component_set = static_cast<IComponentSet<ComponentType>*>(it->second.get());
				component_set->RemoveComponent(entity);
			}
		}

		IEntity CreateEntity()
		{
			if (_free_entity_ids.empty())
			{
				size_t id = _entity_versions.size();
				_entity_versions.push_back(0);
				return IEntity{id, 0};
			}
			else
			{
				size_t id = _free_entity_ids.top();
				_free_entity_ids.pop();
				return IEntity{id, _entity_versions[id]};
			}
		}

		void DestroyEntity(IEntity& entity)
		{
			if (entity)
			{
				if (entity.Id < _entity_versions.size())
				{
					if (entity.Version == _entity_versions[entity.Id])
					{
						// invalidate the entity by incrementing its version and adding it to the free list
						_entity_versions[entity.Id] = (_entity_versions[entity.Id] + 1) == size_t(-1) ? 0 : _entity_versions[entity.Id] + 1;
						_free_entity_ids.push(entity.Id);
					}
				}
			}
		}

		template<typename ComponentType>
		IComponentSet<ComponentType>* GetComponentSet()
		{
			if (HasComponentSet<ComponentType>())
			{
				auto& compSet = _component_sets.find(std::type_index(typeid(ComponentType)))->second;
				return (IComponentSet<ComponentType>*)compSet.get();
			}
			return nullptr;
		}


	private:
		std::unordered_map<std::type_index, std::unique_ptr<IComponentSetBase>> _component_sets;
		std::mutex _mutex;
		// 
		std::vector<size_t> _entity_versions;
		std::stack<size_t> _free_entity_ids;
		std::mutex _entity_mutex;


	};
}
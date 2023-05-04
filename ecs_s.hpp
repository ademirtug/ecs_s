#pragma once

#include <array>
#include <memory>
#include <unordered_map> 

namespace ecs_s {
#define _SPARSE_CAPACITY_ 8192

	using entity = uint64_t;
	using component_id = entity;
	class registry;

	//term: type erasure
	class sparse_base {
	public:
		virtual void erase(const size_t& index) {};
	};

	//term: sparse set
	template<typename T, size_t capacity = _SPARSE_CAPACITY_> class sparse_set : public sparse_base {
		struct storage {
			size_t sparse_index{ UINT64_MAX };
			T payload;
		};
		std::array<size_t, capacity> _sparse;
		std::array<storage, capacity> _dense;
		size_t n{ 0 };
	public:
		sparse_set();

		T& operator[](const size_t& index);
		void insert(const size_t& index, T& t);
		void erase(const size_t& index) override;
		bool has(const size_t& index);
		auto begin();
		auto end();
	};

	class registry {
		//term: recursive variadic helper template structure
		template <size_t N, typename T, typename ...Ts>
		struct component_has_helper {
			static bool component_has_impl(registry& world, const entity& e) {
				return (static_cast<sparse_set<T>*>(world._component_data[world.get_component_id<T>()].get()))->has(e) &&
					component_has_helper<N - 1, Ts...>::component_has_impl(world, e);
			}
		};
		template <typename T>
		struct component_has_helper<0, T> {
			static bool component_has_impl(registry& world, const entity& e) {
				return (static_cast<sparse_set<T>*>(world._component_data[world.get_component_id<T>()].get()))->has(e);
			}
		};
		std::unordered_map<component_id, std::shared_ptr<sparse_base>> _component_data;
	public:
		[[nodiscard]] entity new_entity() {
			static uint64_t entity_counter;
			return ++entity_counter;
		}

		void remove_entity(const entity& e) {
			for (auto it : _component_data) {
				it.second->erase(e);
			}
		}
		//registry impl
		template<typename T>
		inline T& get_component_value_for(const entity& e) {
			return (*(static_cast<sparse_set<T>*>(_component_data[get_component_id<T>()].get())))[e];
		}
		template<typename ...Ts>
		inline bool component_has(const entity& e) {
			return component_has_helper<sizeof...(Ts) - 1, Ts...>::component_has_impl(*this, e);
		}
		//TODO: passing lvalue causes C2280! hence T&&	
		template<typename T>
		void add_component(const entity& e, T&& p) {
			if (_component_data.find(get_component_id<T>()) == _component_data.end()) {
				_component_data[get_component_id<T>()] = std::make_shared<sparse_set<T>>();
			}
			static_cast<sparse_set<T>*>(_component_data[get_component_id<T>()].get())->insert(e, p);
		}
		template<typename T>
		void add_component(const entity& e, T& p) {
			if (_component_data.find(get_component_id<T>()) == _component_data.end()) {
				_component_data[get_component_id<T>()] = std::make_shared<sparse_set<T>>();
			}
			static_cast<sparse_set<T>*>(_component_data[get_component_id<T>()].get())->insert(e, p);
		}
		template<typename T>
		void remove_component(const entity& e) {
			static_cast<sparse_set<T>*>(_component_data[get_component_id<T>()].get())->erase(e);
		}
		//trick: each T will get its own static variable
		template<typename T>
		[[nodiscard]] component_id get_component_id() {
			static component_id id = new_entity();
			return id;
		}
		template<typename T, typename F>
		void each(F f) {
			sparse_set<T>* ssp = static_cast<sparse_set<T>*>(_component_data[get_component_id<T>()].get());
			auto it = ssp->begin();
			while (it++ != ssp->end()) {
				f(it->sparse_index, it->payload);
			}
		}
		template<typename... Ts, typename F>
		void view(F f) {
			using T = std::tuple_element_t<0, std::tuple<Ts...>>;
			sparse_set<T>* ssp = static_cast<sparse_set<T>*>(_component_data[get_component_id<T>()].get());
			if (!ssp) {
				throw std::runtime_error("no such component!: " + std::string(typeid(T).name()));
			}
			for (auto it = ssp->begin(); it != ssp->end(); it++) {
				entity e = it->sparse_index;
				if (!component_has<Ts...>(e))
					continue;
				auto tx = std::move(std::make_tuple(e, get_component_value_for<Ts>(e)...));
				std::apply(f, tx);
			}
		}
	};

	//sparse_set impl
	template<typename T, size_t capacity>
	sparse_set<T, capacity>::sparse_set() {
		_sparse.fill(UINT64_MAX);
	}
	template<typename T, size_t capacity>
	T& sparse_set<T, capacity>::operator[](const size_t& index) {
		return _dense[_sparse[index]].payload;
	}
	template<typename T, size_t capacity>
	void sparse_set<T, capacity>::insert(const size_t& index, T& t) {
		_dense[n] = { index, t };
		_sparse[index] = n++;
	}
	template<typename T, size_t capacity>
	void sparse_set<T, capacity>::erase(const size_t& index) {
		if (!has(index))
			return;
		_dense[_sparse[index]] = _dense[--n];
		_sparse[_dense[n].sparse_index] = index;
		_sparse[index] = UINT64_MAX;
	}
	template<typename T, size_t capacity>
	bool sparse_set<T, capacity>::has(const size_t& index) {
		return _sparse[index] < n &&
			_sparse[index] != UINT64_MAX &&
			_dense[_sparse[index]].sparse_index == index;
	}
	template<typename T, size_t capacity>
	auto sparse_set<T, capacity>::begin() {
		return _dense.begin();
	}
	template<typename T, size_t capacity>
	auto sparse_set<T, capacity>::end() {
		return _dense.begin() + n;
	}


}
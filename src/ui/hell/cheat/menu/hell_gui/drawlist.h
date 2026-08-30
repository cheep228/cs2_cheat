#pragma once

#include <context.h>
#include <functional>

#define ADD_HELL_DRAWLIST( func ) hell::drawlist::add( [ & ] { func; } )

namespace hell {
	namespace drawlist {
		inline std::vector<std::function<void( )>> m_queued;

		inline void add( std::function<void( )> content ) {
			m_queued.emplace_back( content );
		}

		inline void on_present( ) {
			for ( auto& function : m_queued ) {
				if ( function )
					function( );
			}
			m_queued.clear( );
		}
	}
}
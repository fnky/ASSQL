#include <cassert>
#include <cstring>

#include "ASMySQLDateTime.h"

#include "CASMySQLBind.h"
#include "CASMySQLConnection.h"
#include "CASMySQLPreparedStatement.h"

#include "CASMySQLResultSet.h"

CASMySQLResultSet::CASMySQLResultSet( CASMySQLPreparedStatement* pStatement, MYSQL_STMT* pMyStatement )
{
	assert( pStatement );
	assert( pMyStatement );

	m_pStatement = pStatement;

	m_pStatement->AddRef();

	m_pMyStatement = pMyStatement;

	const int iMaxLength = 1;
	mysql_stmt_attr_set( m_pMyStatement, STMT_ATTR_UPDATE_MAX_LENGTH, &iMaxLength );

	if( mysql_stmt_store_result( m_pMyStatement ) != 0 )
	{
		pStatement->GetConnection()->GetLogFunction()( "MySQLResultSet::MySQLResultSet: %s\n", mysql_error( m_pMyStatement->mysql ) );

		Destruct();
	}
	else
	{
		m_pResultSet = mysql_stmt_result_metadata( m_pMyStatement );

		if( m_pResultSet )
		{
			m_pFields = mysql_fetch_fields( m_pResultSet );

			const int iFieldCount = GetFieldCount();

			if( iFieldCount > 0 )
			{
				m_pBinds = new MYSQL_BIND[ iFieldCount ];

				memset( m_pBinds, 0, sizeof( MYSQL_BIND ) * iFieldCount );

				m_pVariables = new CASMySQLBind[ iFieldCount ];

				for( int iIndex = 0; iIndex < iFieldCount; ++iIndex )
				{
					m_pVariables[ iIndex ].SetOutput( m_pFields[ iIndex ], &m_pBinds[ iIndex ] );
				}

				mysql_stmt_bind_result( m_pMyStatement, m_pBinds );
			}
		}
	}
}

CASMySQLResultSet::~CASMySQLResultSet()
{
	Destruct();

	m_pStatement->Release();
}

void CASMySQLResultSet::Destruct()
{
	if( m_pResultSet )
	{
		mysql_free_result( m_pResultSet );
		m_pResultSet = nullptr;
	}

	mysql_stmt_free_result( m_pMyStatement );

	if( m_pBinds )
	{
		delete[] m_pBinds;
		m_pBinds = nullptr;
	}

	if( m_pVariables )
	{
		delete[] m_pVariables;
		m_pVariables = nullptr;
	}
}

void CASMySQLResultSet::CallbackInvoked()
{
	m_pStatement->m_bHandledResultSet = true;
}

bool CASMySQLResultSet::IsValid() const
{
	return m_pResultSet != nullptr;
}

uint32_t CASMySQLResultSet::GetFieldCount() const
{
	return static_cast<uint32_t>( mysql_stmt_field_count( m_pMyStatement ) );
}

bool CASMySQLResultSet::Next()
{
	return mysql_stmt_fetch( m_pMyStatement ) == 0;
}

enum_field_types CASMySQLResultSet::GetColumnType( const uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return MAX_NO_FIELD_TYPES;

	return m_pBinds[ uiColumn ].buffer_type;
}

bool CASMySQLResultSet::IsNull( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return true;

	return m_pVariables[ uiColumn ].m_bIsNull != 0;
}

bool CASMySQLResultSet::GetBoolean( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return m_pVariables[ uiColumn ].m_uiVal64 != 0;
}

int8_t CASMySQLResultSet::GetSigned8( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return static_cast<int8_t>( m_pVariables[ uiColumn ].m_iVal64 );
}

uint8_t CASMySQLResultSet::GetUnsigned8( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return static_cast<uint8_t>( m_pVariables[ uiColumn ].m_uiVal64 );
}

int16_t CASMySQLResultSet::GetSigned16( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return static_cast<int16_t>( m_pVariables[ uiColumn ].m_iVal64 );
}

uint16_t CASMySQLResultSet::GetUnsigned16( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return static_cast<uint16_t>( m_pVariables[ uiColumn ].m_uiVal64 );
}

int32_t CASMySQLResultSet::GetSigned32( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return static_cast<int32_t>( m_pVariables[ uiColumn ].m_iVal64 );
}

uint32_t CASMySQLResultSet::GetUnsigned32( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return static_cast<uint32_t>( m_pVariables[ uiColumn ].m_uiVal64 );
}

int64_t CASMySQLResultSet::GetSigned64( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return m_pVariables[ uiColumn ].m_iVal64;
}

uint64_t CASMySQLResultSet::GetUnsigned64( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return m_pVariables[ uiColumn ].m_uiVal64;
}

float CASMySQLResultSet::GetFloat( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return m_pVariables[ uiColumn ].m_flValue32[ 0 ];
}

double CASMySQLResultSet::GetDouble( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return 0;

	return m_pVariables[ uiColumn ].m_flValue64;
}

std::string CASMySQLResultSet::GetString( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return "";

	auto& buffer = m_pVariables[ uiColumn ].m_Buffer;

	return !buffer.empty() ? std::string( buffer.begin(), buffer.end() ) : "";
}

CASDateTime CASMySQLResultSet::GetDate( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return CASDateTime();

	return CASDateTime_FromMySQLTime( m_pVariables[ uiColumn ].m_Time );
}

CASTime CASMySQLResultSet::GetTime( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return CASDateTime();

	return CASTime_FromMySQLTime( m_pVariables[ uiColumn ].m_Time );
}

CASDateTime CASMySQLResultSet::GetDateTime( uint32_t uiColumn ) const
{
	if( uiColumn >= GetFieldCount() )
		return CASDateTime();

	return CASDateTime_FromMySQLTime( m_pVariables[ uiColumn ].m_Time );
}

//-*****************************************************************************
//
// Copyright (c) 2013,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include <Alembic/AbcCoreOgawa/OrData.h>
#include <Alembic/AbcCoreOgawa/OrImpl.h>
#include <Alembic/AbcCoreOgawa/CprData.h>
#include <Alembic/AbcCoreOgawa/CprImpl.h>
#include <Alembic/AbcCoreOgawa/ReadUtil.h>

namespace Alembic {
namespace AbcCoreOgawa {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
OrData::OrData( Ogawa::IGroupPtr iGroup,
                const std::string & iParentName,
                std::size_t iThreadId,
                int32_t iArchiveVersion )
{
    ABCA_ASSERT( iGroup, "Invalid object data group" );

    m_group = iGroup;

    std::size_t numChildren = m_group->getNumChildren();

    if ( numChildren > 0 && m_group->isChildData( numChildren - 1 ) )
    {
        std::vector< ObjectHeaderPtr > headers;
        ReadObjectHeaders( m_group, numChildren - 1, iThreadId,
                           iParentName, headers );

        m_children.resize( headers.size() );
        for ( std::size_t i = 0; i < headers.size(); ++i )
        {
            m_subProperties[headers[i]->getName()] = i;
            m_children[i].header = headers[i];
        }
    }

    if ( numChildren > 0 && m_group->isChildGroup( 0 ) )
    {
        Ogawa::GroupPtr group = m_group->getGroup( 0, iThreadId );
        m_data.reset( new CprData( group, iArchiveVersion, iThreadId ) );
    }
}

//-*****************************************************************************
OrData::~OrData()
{
}

//-*****************************************************************************
AbcA::CompoundPropertyReaderPtr
OrData::getProperties( AbcA::ObjectReaderPtr iParent )
{
    AbcA::CompoundPropertyReaderPtr ret = m_top.lock();
    if ( ! ret )
    {
        // time to make a new one
        ret.reset( new CprImpl( iParent, m_data ) );
        m_top = ret;
    }

    return ret;
}

//-*****************************************************************************
size_t OrData::getNumChildren()
{
    return m_children.size();
}

//-*****************************************************************************
const AbcA::ObjectHeader &
OrData::getChildHeader( AbcA::ObjectReaderPtr iParent, size_t i )
{
    ABCA_ASSERT( i < m_children.size(),
        "Out of range index in OrData::getChildHeader: " << i );

    return *( m_children[i].header );
}

//-*****************************************************************************
const AbcA::ObjectHeader *
OrData::getChildHeader( AbcA::ObjectReaderPtr iParent,
                        const std::string &iName )
{
    ChildrenMap::iterator fiter = m_childrenMap.find( iName );
    if ( fiter == m_childrenMap.end() )
    {
        return NULL;
    }

    return & getChildHeader( iParent, fiter->second );
}

//-*****************************************************************************
AbcA::ObjectReaderPtr
OrData::getChild( AbcA::ObjectReaderPtr iParent, const std::string &iName )
{
    ChildrenMap::iterator fiter = m_childrenMap.find( iName );
    if ( fiter == m_childrenMap.end() )
    {
        return AbcA::ObjectReaderPtr();
    }

    return getChild( iParent, fiter->second );
}

//-*****************************************************************************
AbcA::ObjectReaderPtr
OrData::getChild( AbcA::ObjectReaderPtr iParent, size_t i )
{
    ABCA_ASSERT( i < m_children.size(),
        "Out of range index in OrData::getChild: " << i );

    AbcA::ObjectReaderPtr optr = m_children[i].made.lock();
    if ( ! optr )
    {
        // TODO get thread id from archive on parent
        // (dynamic cast optr->getArchive() to ArImpl?)
        // "top" compound is at the 0 position
        Ogawa::IGroupPtr group = m_group.getGroup( i + 1, 0 );
        // Make a new one.
        optr.reset ( new OrImpl( group, iParent, group,
                                 m_children[i].header ) );
        m_children[i].made = optr;
    }
    return optr;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreOgawa
} // End namespace Alembic

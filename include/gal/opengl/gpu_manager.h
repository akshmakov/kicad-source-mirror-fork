/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef GPU_MANAGER_H_
#define GPU_MANAGER_H_

#include <gal/opengl/vertex_common.h>
#include <boost/scoped_array.hpp>

namespace KIGFX
{
class SHADER;
class VERTEX_CONTAINER;
class CACHED_CONTAINER;
class NONCACHED_CONTAINER;

/**
 * @brief Class to handle uploading vertices and indices to GPU in drawing purposes.
 */

class GPU_MANAGER
{
public:
    static GPU_MANAGER* MakeManager( VERTEX_CONTAINER* aContainer );

    virtual ~GPU_MANAGER();

    /**
     * Function BeginDrawing()
     * Prepares the stored data to be drawn.
     */
    virtual void BeginDrawing() = 0;

    /**
     * Function DrawIndices()
     * Makes the GPU draw given range of vertices.
     * @param aOffset is the beginning of the range.
     * @param aSize is the number of vertices to be drawn.
     */
    virtual void DrawIndices( unsigned int aOffset, unsigned int aSize ) = 0;

    /**
     * Function DrawIndices()
     * Makes the GPU draw all the vertices stored in the container.
     */
    virtual void DrawAll() = 0;

    /**
     * Function EndDrawing()
     * Clears the container after drawing routines.
     */
    virtual void EndDrawing() = 0;

    /**
     * Function SetShader()
     * Allows using shaders with the stored data.
     * @param aShader is the object that allows using shaders.
     */
    virtual void SetShader( SHADER& aShader );

    /**
     * Function EnableDepthTest()
     * Enables/disables Z buffer depth test.
     */
    void EnableDepthTest( bool aEnabled );

protected:
    GPU_MANAGER( VERTEX_CONTAINER* aContainer );

    ///> Drawing status flag.
    bool m_isDrawing;

    ///> Container that stores vertices data.
    VERTEX_CONTAINER* m_container;

    ///> Shader handling
    SHADER* m_shader;

    ///> Location of shader attributes (for glVertexAttribPointer)
    int m_shaderAttrib;

    ///> true: enable Z test when drawing
    bool m_enableDepthTest;
};


class GPU_CACHED_MANAGER : public GPU_MANAGER
{
public:
    GPU_CACHED_MANAGER( VERTEX_CONTAINER* aContainer );
    ~GPU_CACHED_MANAGER();

    ///> @copydoc GPU_MANAGER::BeginDrawing()
    virtual void BeginDrawing() override;

    ///> @copydoc GPU_MANAGER::DrawIndices()
    virtual void DrawIndices( unsigned int aOffset, unsigned int aSize ) override;

    ///> @copydoc GPU_MANAGER::DrawAll()
    virtual void DrawAll() override;

    ///> @copydoc GPU_MANAGER::EndDrawing()
    virtual void EndDrawing() override;

    ///> Maps vertex buffer stored in GPU memory.
    void Map();

    ///> Unmaps vertex buffer.
    void Unmap();

protected:
    ///> Resizes the indices buffer to aNewSize if necessary
    void resizeIndices( unsigned int aNewSize );

    ///> Buffers initialization flag
    bool m_buffersInitialized;

    ///> Pointer to the current indices buffer
    boost::scoped_array<GLuint> m_indices;

    ///> Pointer to the first free cell in the indices buffer
    GLuint* m_indicesPtr;

    ///> Handle to indices buffer
    GLuint  m_indicesBuffer;

    ///> Number of indices stored in the indices buffer
    unsigned int m_indicesSize;

    ///> Current indices buffer size
    unsigned int m_indicesCapacity;
};


class GPU_NONCACHED_MANAGER : public GPU_MANAGER
{
public:
    GPU_NONCACHED_MANAGER( VERTEX_CONTAINER* aContainer );

    ///> @copydoc GPU_MANAGER::BeginDrawing()
    virtual void BeginDrawing() override;

    ///> @copydoc GPU_MANAGER::DrawIndices()
    virtual void DrawIndices( unsigned int aOffset, unsigned int aSize ) override;

    ///> @copydoc GPU_MANAGER::DrawAll()
    virtual void DrawAll() override;

    ///> @copydoc GPU_MANAGER::EndDrawing()
    virtual void EndDrawing() override;
};
} // namespace KIGFX
#endif /* GPU_MANAGER_H_ */

/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "common.h"
#include "bgfx_utils.h"

#include <vector>


//#define USE_STATIC_BUFFER

#ifdef USE_STATIC_BUFFER
#define VERTEX_BUFFER bgfx::VertexBufferHandle
#define INDEX_BUFFER bgfx::IndexBufferHandle
#define CREATE_VERTEX bgfx::createVertexBuffer
#define CREATE_INDEX bgfx::createIndexBuffer
#define DESTORY_VERTEX bgfx::destroyVertexBuffer
#define DESTORY_INDEX bgfx::destroyIndexBuffer
#else
#define VERTEX_BUFFER bgfx::DynamicVertexBufferHandle
#define INDEX_BUFFER bgfx::DynamicIndexBufferHandle
#define CREATE_VERTEX bgfx::createDynamicVertexBuffer
#define CREATE_INDEX bgfx::createDynamicIndexBuffer
#define DESTORY_VERTEX bgfx::destroyDynamicVertexBuffer
#define DESTORY_INDEX bgfx::destroyDynamicIndexBuffer
#endif


struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosColorVertex::ms_decl;

static PosColorVertex s_cubeVertices[8] =
{
	{-1.0f,  1.0f,  1.0f, 0xff000000 },
	{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
	{-1.0f, -1.0f,  1.0f, 0xff00ff00 },
	{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
	{-1.0f,  1.0f, -1.0f, 0xffff0000 },
	{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
	{-1.0f, -1.0f, -1.0f, 0xffffff00 },
	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeIndices[36] =
{
	0, 1, 2, // 0
	1, 3, 2,
	4, 6, 5, // 2
	5, 6, 7,
	0, 2, 4, // 4
	4, 2, 6,
	1, 5, 3, // 6
	5, 7, 3,
	0, 4, 1, // 8
	4, 5, 1,
	2, 3, 6, // 10
	6, 3, 7,
};

static PosColorVertex s_quadVertices[4] =
{
	{ -1.0f, 1.0f, 0.0f, 0xff000000 },
	{ 1.0f, 1.0f, 0.0f, 0xff0000ff },
	{ -1.0f, -1.0f, 0.0f, 0xff00ff00 },
	{ 1.0f, -1.0f, 0.0f, 0xff00ffff },
};

static const uint16_t s_quadIndices[6] =
{
	0, 1, 2, // 0
	1, 3, 2,
};

class Cubes : public entry::AppI
{
	void init(int /*_argc*/, char** /*_argv*/) BX_OVERRIDE
	{
		m_width  = 1280;
		m_height = 720;
		m_debug = BGFX_DEBUG_TEXT;
		m_reset = BGFX_RESET_VSYNC;

		bgfx::init();
		bgfx::reset(m_width, m_height, m_reset);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
				);

		// Create vertex stream declaration.
		PosColorVertex::init();

		m_objCnt = 0;

		// Create program from shaders.
		m_program = loadProgram("vs_cubes", "fs_cubes");

		m_timeOffset = bx::getHPCounter();
	}

	virtual int shutdown() BX_OVERRIDE
	{
		for (int i = 0; i < m_objCnt; ++i)
		{
			DESTORY_INDEX(m_dibhVtr[i]);
			DESTORY_VERTEX(m_dvbhVtr[i]);
		}


		// Cleanup.
		bgfx::destroyProgram(m_program);

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	void createCube()
	{
		// create buffer handle
		INDEX_BUFFER _dibh = CREATE_INDEX(
			bgfx::makeRef(s_cubeIndices, sizeof(s_cubeIndices))
			);

		VERTEX_BUFFER _dvbh = CREATE_VERTEX(
			bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), PosColorVertex::ms_decl
			);

		m_dibhVtr.push_back(_dibh);
		m_dvbhVtr.push_back(_dvbh);

		m_objCnt++;
	}

	bool update() BX_OVERRIDE
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset) )
		{
			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency() );
			const double toMs = 1000.0/freq;

			float time = (float)( (now-m_timeOffset)/double(bx::getHPFrequency() ) );

			// Use debug font to print information about this example.
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/01-cube");
			bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Rendering simple static mesh.");
			bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

			float at[3]  = { 0.0f, 0.0f,   0.0f };
			float eye[3] = { 0.0f, 0.0f, -35.0f };

			// Set view and projection matrix for view 0.
			const bgfx::HMD* hmd = bgfx::getHMD();
			if (NULL != hmd && 0 != (hmd->flags & BGFX_HMD_RENDERING) )
			{
				float view[16];
				bx::mtxQuatTranslationHMD(view, hmd->eye[0].rotation, eye);

				float proj[16];
				bx::mtxProj(proj, hmd->eye[0].fov, 0.1f, 100.0f);

				bgfx::setViewTransform(0, view, proj);

				// Set view 0 default viewport.
				//
				// Use HMD's width/height since HMD's internal frame buffer size
				// might be much larger than window size.
				bgfx::setViewRect(0, 0, 0, hmd->width, hmd->height);
			}
			else
			{
				float view[16];
				bx::mtxLookAt(view, eye, at);

				float proj[16];
				bx::mtxProj(proj, 60.0f, float(m_width)/float(m_height), 0.1f, 100.0f);
				bgfx::setViewTransform(0, view, proj);

				// Set view 0 default viewport.
				bgfx::setViewRect(0, 0, 0, m_width, m_height);
			}

			// This dummy draw call is here to make sure that view 0 is cleared
			// if no other draw calls are submitted to view 0.
			bgfx::touch(0);

			if (m_objCnt < 121)
			{
				if (time > m_objCnt)
				{
					//if (m_objCnt % 2 == 0)
					{
						createCube();
					}
				}
			}

			for (uint32_t yy = 0; yy < 11; ++yy)
			{
				for (uint32_t xx = 0; xx < 11; ++xx)
				{
					int _currIdx = yy * 11 + xx;

					if (_currIdx >= m_objCnt) break;

					float mtx[16];
					bx::mtxRotateXY(mtx, time + xx*0.21f, time + yy*0.37f);
					mtx[12] = -15.0f + float(xx)*3.0f;
					mtx[13] = -15.0f + float(yy)*3.0f;
					mtx[14] = 0.0f;

					// Set model matrix for rendering.
					bgfx::setTransform(mtx);

					// Set vertex and index buffer.
					bgfx::setVertexBuffer(m_dvbhVtr[_currIdx]);
					bgfx::setIndexBuffer(m_dibhVtr[_currIdx]);

					// Set render states.
					bgfx::setState(BGFX_STATE_DEFAULT);

					// Submit primitive for rendering to view 0.
					bgfx::submit(0, m_program);
				}
			}


			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	bgfx::ProgramHandle m_program;
	int64_t m_timeOffset;

	
	std::vector<VERTEX_BUFFER> m_dvbhVtr;
	std::vector<INDEX_BUFFER> m_dibhVtr;
	uint32_t m_objCnt;
};

ENTRY_IMPLEMENT_MAIN(Cubes);

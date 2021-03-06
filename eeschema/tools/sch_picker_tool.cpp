/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_picker_tool.h>
#include <sch_actions.h>
#include <view/view_controls.h>
#include <tool/tool_manager.h>
#include <sch_base_frame.h>

TOOL_ACTION SCH_ACTIONS::pickerTool( "eeschema.Picker", AS_GLOBAL, 0, "", "", NULL, AF_ACTIVATE );


SCH_PICKER_TOOL::SCH_PICKER_TOOL()
    : TOOL_INTERACTIVE( "eeschema.Picker" )
{
    reset();
}


int SCH_PICKER_TOOL::Main( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    int finalize_state = WAIT_CANCEL;

    setControls();

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = controls->GetCursorPosition( true );

        if( evt->IsClick( BUT_LEFT ) )
        {
            bool getNext = false;

            m_picked = cursorPos;

            if( m_clickHandler )
            {
                try
                {
                    getNext = (*m_clickHandler)( *m_picked );
                }
                catch( std::exception& e )
                {
                    std::cerr << "SCH_PICKER_TOOL click handler error: " << e.what() << std::endl;
                    finalize_state = EXCEPTION_CANCEL;
                    break;
                }
            }

            if( !getNext )
            {
                finalize_state = CLICK_CANCEL;
                break;
            }
            else
                setControls();
        }

        else if( evt->IsAction( &ACTIONS::cancelInteractive ) || evt->IsActivate() || evt->IsCancel() )
        {
            if( m_cancelHandler )
            {
                try
                {
                    (*m_cancelHandler)();
                }
                catch( std::exception& e )
                {
                    std::cerr << "SCH_PICKER_TOOL cancel handler error: " << e.what() << std::endl;
                }
            }

            // Activating a new tool may have alternate finalization from canceling the current tool
            if( evt->IsActivate() )
                finalize_state = END_ACTIVATE;
            else
                finalize_state = EVT_CANCEL;

            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // TODO...
            // m_menu.ShowContextMenu();
        }
        else
        {
            m_toolMgr->PassEvent();
        }
    }

    if( m_finalizeHandler )
    {
        try
        {
            (*m_finalizeHandler)( finalize_state );
        }
        catch( std::exception& e )
        {
            std::cerr << "SCH_PICKER_TOOL finalize handler error: " << e.what() << std::endl;
        }
    }

    reset();
    controls->ForceCursorPosition( false );
    getEditFrame<SCH_BASE_FRAME>()->SetNoToolSelected();

    return 0;
}


void SCH_PICKER_TOOL::setTransitions()
{
    Go( &SCH_PICKER_TOOL::Main, SCH_ACTIONS::pickerTool.MakeEvent() );
}


void SCH_PICKER_TOOL::reset()
{
    m_cursorCapture = false;
    m_autoPanning = false;

    m_picked = NULLOPT;
    m_clickHandler = NULLOPT;
    m_cancelHandler = NULLOPT;
    m_finalizeHandler = NULLOPT;
}


void SCH_PICKER_TOOL::setControls()
{
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    controls->CaptureCursor( m_cursorCapture );
    controls->SetAutoPan( m_autoPanning );
}

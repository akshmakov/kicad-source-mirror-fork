/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2012-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file toolbars_update_user_interface.cpp
 * @brief Function to update toolbars UI after changing parameters.
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <class_board.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <drc.h>
#include <pcb_layer_box_selector.h>


void PCB_EDIT_FRAME::OnUpdateLayerPair( wxUpdateUIEvent& aEvent )
{
    PrepareLayerIndicator();
}


void PCB_EDIT_FRAME::OnUpdateSelectTrackWidth( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_TRACK_WIDTH )
    {
        if( m_SelTrackWidthBox->GetSelection() != (int) GetDesignSettings().GetTrackWidthIndex() )
            m_SelTrackWidthBox->SetSelection( GetDesignSettings().GetTrackWidthIndex() );
    }
}


void PCB_EDIT_FRAME::OnUpdateSelectViaSize( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetId() == ID_AUX_TOOLBAR_PCB_VIA_SIZE )
    {
        if( m_SelViaSizeBox->GetSelection() != (int) GetDesignSettings().GetViaSizeIndex() )
            m_SelViaSizeBox->SetSelection( GetDesignSettings().GetViaSizeIndex() );
    }
}


void PCB_EDIT_FRAME::OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent )
{
    m_SelLayerBox->SetLayerSelection( GetActiveLayer() );
}


#if defined( KICAD_SCRIPTING_WXPYTHON )

// Used only when the DKICAD_SCRIPTING_WXPYTHON option is on
void PCB_EDIT_FRAME::OnUpdateScriptingConsoleState( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() != m_mainToolBar )
        return;

    wxMiniFrame* pythonPanelFrame = (wxMiniFrame *) findPythonConsole();
    bool pythonPanelShown = pythonPanelFrame ? pythonPanelFrame->IsShown() : false;
    aEvent.Check( pythonPanelShown );
}

#endif


void PCB_EDIT_FRAME::OnUpdateZoneDisplayStyle( wxUpdateUIEvent& aEvent )
{

}


void PCB_EDIT_FRAME::OnUpdateDrcEnable( wxUpdateUIEvent& aEvent )
{
    bool state = !Settings().m_legacyDrcOn;
    aEvent.Check( state );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DRC_OFF,
                                        Settings().m_legacyDrcOn ?
                                        _( "Disable design rule checking while routing/editing tracks using Legacy Toolset.\nUse Route > Interactive Router Settings... for Modern Toolset." ) :
                                        _( "Enable design rule checking while routing/editing tracks using Legacy Toolset.\nUse Route > Interactive Router Settings... for Modern Toolset." ) );
}

void PCB_EDIT_FRAME::OnUpdateShowBoardRatsnest( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( GetBoard()->IsElementVisible( LAYER_RATSNEST ) );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_RATSNEST,
                                        GetBoard()->IsElementVisible( LAYER_RATSNEST ) ?
                                        _( "Hide board ratsnest" ) :
                                        _( "Show board ratsnest" ) );
}


void PCB_EDIT_FRAME::OnUpdateViaDrawMode( wxUpdateUIEvent& aEvent )
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    aEvent.Check( !displ_opts->m_DisplayViaFill );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_VIAS_SKETCH,
                                        displ_opts->m_DisplayViaFill ?
                                        _( "Show vias in outline mode" ) :
                                        _( "Show vias in fill mode" ) );
}


void PCB_EDIT_FRAME::OnUpdateTraceDrawMode( wxUpdateUIEvent& aEvent )
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    aEvent.Check( !displ_opts->m_DisplayPcbTrackFill );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                        displ_opts->m_DisplayPcbTrackFill ?
                                        _( "Show tracks in outline mode" ) :
                                        _( "Show tracks in fill mode" ) );
}


void PCB_EDIT_FRAME::OnUpdateHighContrastDisplayMode( wxUpdateUIEvent& aEvent )
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    aEvent.Check( displ_opts->m_ContrastModeDisplay );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE,
                                        displ_opts->m_ContrastModeDisplay ?
                                        _( "Normal contrast display mode" ) :
                                        _( "High contrast display mode" ) );
}

void PCB_EDIT_FRAME::OnUpdateCurvedRatsnest( wxUpdateUIEvent& aEvent )
{
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    aEvent.Check( displ_opts->m_DisplayRatsnestLinesCurved );
    m_optionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_CURVED_RATSNEST_LINES,
                                        !displ_opts->m_DisplayRatsnestLinesCurved ?
                                        _( "Show ratsnest with curved lines" ) :
                                        _( "Show ratsnest with straight lines" ) );
}


void PCB_EDIT_FRAME::OnUpdateShowLayerManager( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_auimgr.GetPane( "LayersManager" ).IsShown() );
}

void PCB_EDIT_FRAME::OnUpdateShowMicrowaveToolbar( wxUpdateUIEvent& aEvent )
{
    aEvent.Check( m_auimgr.GetPane( "MicrowaveToolbar" ).IsShown() );
}


void PCB_EDIT_FRAME::OnUpdateSave( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetScreen()->IsModify() );
}


void PCB_EDIT_FRAME::OnUpdateVerticalToolbar( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_drawToolBar || aEvent.GetEventObject() == m_mainToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}

void PCB_EDIT_FRAME::OnUpdateMuWaveToolbar( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_microWaveToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void PCB_EDIT_FRAME::SyncMenusAndToolbars( wxEvent& aEvent )
{
    auto displOpts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();
    auto menuBar = GetMenuBar();

    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES, false );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES_DISABLE, false );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY, false );

    switch( displOpts->m_DisplayZonesMode )
    {
        case 0:
            menuBar->FindItem( ID_TB_OPTIONS_SHOW_ZONES )->Check( true );
            m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES, true );
            break;

        case 1:
            menuBar->FindItem( ID_TB_OPTIONS_SHOW_ZONES_DISABLE )->Check( true );
            m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES_DISABLE, true );
            break;

        case 2:
            menuBar->FindItem( ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY )->Check( true );
            m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY, true );
            break;
    }

    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM, false );
    m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, false );

    if( GetUserUnits() == INCHES )
    {
        menuBar->FindItem( ID_TB_OPTIONS_SELECT_UNIT_INCH )->Check( true );
        m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, true );
    }
    else
    {
        menuBar->FindItem( ID_TB_OPTIONS_SELECT_UNIT_MM )->Check( true );
        m_optionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM, true );
    }
}

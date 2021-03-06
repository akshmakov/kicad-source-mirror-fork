 /*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <gr_basic.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <sch_edit_frame.h>
#include <hotkeys_basic.h>
#include <general.h>
#include <eeschema_id.h>
#include <list_operations.h>
#include <class_library.h>
#include <sch_bus_entry.h>
#include <sch_marker.h>
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_view.h>
#include <simulation_cursors.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>

 void SCH_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int         id = event.GetId();
    wxPoint     pos;
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    pos = wxGetMousePosition();

    pos.y += 20;

    // If needed, stop the current command and deselect current tool
    switch( id )
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_POPUP_CANCEL_CURRENT_COMMAND:
    case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
    case ID_POPUP_SCH_BEGIN_WIRE:
    case ID_POPUP_SCH_BEGIN_BUS:
    case ID_POPUP_END_LINE:
    case ID_POPUP_SCH_CLEANUP_SHEET:
    case ID_POPUP_SCH_END_SHEET:
    case ID_POPUP_SCH_RESIZE_SHEET:
    case ID_POPUP_IMPORT_HLABEL_TO_SHEETPIN:
    case ID_POPUP_SCH_INIT_CMP:
    case ID_POPUP_SCH_EDIT_CONVERT_CMP:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_DRAG_BLOCK:
    case ID_POPUP_DUPLICATE_BLOCK:
    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
    case ID_POPUP_SCH_ENTER_SHEET:
    case ID_POPUP_SCH_LEAVE_SHEET:
    case ID_POPUP_SCH_ADD_JUNCTION:
    case ID_POPUP_SCH_ADD_LABEL:
    case ID_POPUP_SCH_GETINFO_MARKER:
        /* At this point: Do nothing. these commands do not need to stop the
         * current command (mainly a block command) or reset the current state
         * They will be executed later, in next switch structure.
         */
        break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:
        // Stop the current command (if any) but keep the current tool
        m_canvas->EndMouseCapture();
        break;

    default:
        // Stop the current command and deselect the current tool
        SetNoToolSelected();
        break;
    }

    item = screen->GetCurItem();    // Can be modified by previous calls.

    switch( id )
    {
    case ID_HIERARCHY:
        InstallHierarchyFrame( pos );
        SetRepeatItem( NULL );
        break;

    case wxID_CUT: // save and delete block
    case ID_POPUP_CUT_BLOCK:

        if( screen->m_BlockLocate.GetCommand() != BLOCK_MOVE )
            break;
        screen->m_BlockLocate.SetCommand( BLOCK_CUT );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        SetRepeatItem( NULL );
        SetSheetNumberAndCount();
        break;

    case wxID_COPY:         // really this is a Save block for paste
    case ID_POPUP_COPY_BLOCK:
        screen->m_BlockLocate.SetCommand( BLOCK_COPY );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        break;

    case wxID_PASTE:
    case ID_POPUP_PASTE_BLOCK:
        HandleBlockBegin( nullptr, BLOCK_PASTE, GetCrossHairPosition() );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_SLASH:
        m_canvas->MoveCursorToCrossHair();
        SetBusEntryShape( nullptr, dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ), '/' );
        break;

    case ID_POPUP_SCH_ENTRY_SELECT_ANTISLASH:
        m_canvas->MoveCursorToCrossHair();
        SetBusEntryShape( nullptr, dynamic_cast<SCH_BUS_ENTRY_BASE*>( item ), '\\' );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->EndMouseCapture();
            SetToolID( GetToolId(), GetGalCanvas()->GetCurrentCursor(), wxEmptyString );
        }
        else
        {
            SetNoToolSelected();
        }
        break;

    case ID_POPUP_END_LINE:
    case ID_POPUP_SCH_END_SHEET:
        m_toolManager->RunAction( SCH_ACTIONS::finishDrawing, true );
        break;

    case ID_POPUP_SCH_DELETE_NODE:
    case ID_POPUP_SCH_DELETE_CONNECTION:
        m_canvas->MoveCursorToCrossHair();
        DeleteConnection( id == ID_POPUP_SCH_DELETE_CONNECTION );
        SchematicCleanUp( true );
        screen->SetCurItem( NULL );
        SetRepeatItem( NULL );

        TestDanglingEnds();
        m_canvas->Refresh();

        break;

    case ID_POPUP_SCH_BREAK_WIRE:
        m_canvas->MoveCursorToCrossHair();
        BreakSegments( GetCrossHairPosition() );
        TestDanglingEnds();
        m_canvas->Refresh();

        break;

    case ID_POPUP_SCH_DELETE_CMP:
    case ID_POPUP_SCH_DELETE:
        if( item == NULL )
            break;

        DeleteItem( item );
        SchematicCleanUp( true );
        screen->SetCurItem( NULL );
        SetRepeatItem( NULL );
        TestDanglingEnds();
        SetSheetNumberAndCount();
        GetCanvas()->Refresh();
        OnModify();
        break;

    case ID_POPUP_SCH_CLEANUP_SHEET:
        if( item != NULL && item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;

            if( !sheet->HasUndefinedPins() )
            {
                DisplayInfoMessage( this, _( "There are no undefined labels in this sheet to clean up." ) );
                return;
            }

            if( !IsOK( this, _( "Do you wish to cleanup this sheet?" ) ) )
                return;

            /* Save sheet in undo list before cleaning up unreferenced hierarchical labels. */
            SaveCopyInUndoList( sheet, UR_CHANGED );
            sheet->CleanupSheet();
            SyncView();
            GetCanvas()->Refresh();
            OnModify();
        }
        break;

    case ID_POPUP_SCH_INIT_CMP:
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_SCH_EDIT_CONVERT_CMP:
        // Ensure the struct is a component (could be a struct of a component, like Field, text..)
        if( item && item->Type() == SCH_COMPONENT_T )
        {
            m_canvas->MoveCursorToCrossHair();
            ConvertPart( (SCH_COMPONENT*) item );
        }
        break;

    case ID_POPUP_SCH_ENTER_SHEET:

        if( item && (item->Type() == SCH_SHEET_T) )
        {
            g_CurrentSheet->push_back( (SCH_SHEET*) item );
            DisplayCurrentSheet();
        }
        break;

    case ID_POPUP_SCH_LEAVE_SHEET:
        if( g_CurrentSheet->Last() != g_RootSheet )
        {
            g_CurrentSheet->pop_back();
            DisplayCurrentSheet();
        }
        break;

    case ID_POPUP_PLACE_BLOCK:
        m_canvas->SetAutoPanRequest( false );
        m_canvas->MoveCursorToCrossHair();
        HandleBlockPlace( nullptr );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        screen->m_BlockLocate.SetCommand( BLOCK_ZOOM );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        break;

    case ID_POPUP_DELETE_BLOCK:
        if( screen->m_BlockLocate.GetCommand() != BLOCK_MOVE )
            break;

        m_canvas->MoveCursorToCrossHair();
        screen->m_BlockLocate.SetCommand( BLOCK_DELETE );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        SetSheetNumberAndCount();
        break;

    case ID_POPUP_DUPLICATE_BLOCK:
        if( screen->m_BlockLocate.GetCommand() != BLOCK_MOVE )
            break;

        m_canvas->MoveCursorToCrossHair();
        screen->m_BlockLocate.SetCommand( BLOCK_DUPLICATE );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        break;

    case ID_POPUP_DRAG_BLOCK:
        if( screen->m_BlockLocate.GetCommand() != BLOCK_MOVE )
            break;

        m_canvas->MoveCursorToCrossHair();
        screen->m_BlockLocate.SetCommand( BLOCK_DRAG );
        screen->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( nullptr );
        break;

    case ID_POPUP_SCH_GETINFO_MARKER:
        if( item && item->Type() == SCH_MARKER_T )
            ( (SCH_MARKER*) item )->DisplayMarkerInfo( this );

        break;

    default:        // Log error:
        wxFAIL_MSG( wxString::Format( "Cannot process command event ID %d", event.GetId() ) );
        break;
    }

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        SetRepeatItem( NULL );
}


void SCH_EDIT_FRAME::OnUnfoldBus( wxCommandEvent& event )
{
     wxMenuItem*     item = static_cast< wxMenuItem* >( event.GetEventUserData() );
     static wxString net = item->GetItemLabelText();

     GetToolManager()->RunAction( SCH_ACTIONS::unfoldBus, true, &net );
}


void SCH_EDIT_FRAME::OnDuplicateItem( wxCommandEvent& event )
{
    SCH_ITEM* curr_item = GetScreen()->GetCurItem();

    if( !curr_item || curr_item->GetEditFlags() )
        return;

    GetCanvas()->GetViewControls()->SetCursorPosition( GetCrossHairPosition() );

    switch( curr_item->Type() )
    {
    case SCH_COMPONENT_T:
    {
         SCH_COMPONENT* newitem = new SCH_COMPONENT( *( (SCH_COMPONENT*) curr_item ) );
         newitem->SetTimeStamp( GetNewTimeStamp() );
         newitem->ClearAnnotation( NULL );
         newitem->SetFlags( IS_NEW );
         // Draw the new part, MoveItem() expects it to be already on screen.
         PrepareMoveItem( newitem );
    }
        break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    {
        SCH_TEXT* newitem = (SCH_TEXT*) curr_item->Clone();
        newitem->SetFlags( IS_NEW );
        // Draw the new item, MoveItem() expects it to be already on screen.
        PrepareMoveItem( newitem );
    }
        break;

    default:
        break;
    }
}


void SCH_EDIT_FRAME::OnMoveItem( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SCREEN*         screen = GetScreen();
    SCH_ITEM*           item = screen->GetCurItem();

    // trying to move an item when there is a block at the same time is not acceptable
    if( screen->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        return;

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = selTool->SelectPoint( data->GetPosition(), SCH_COLLECTOR::MovableItems );

        // Exit if no item found at the current location or the item is already being edited.
        if( item == NULL || item->GetEditFlags() != 0 )
            return;
    }

    SetCrossHairPosition( item->GetPosition(), false );

    GetCanvas()->GetViewControls()->WarpCursor( GetCrossHairPosition(), true );

    switch( item->Type() )
    {
    case SCH_LINE_T:
        break;

    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_TEXT_T:
    case SCH_COMPONENT_T:
    case SCH_SHEET_PIN_T:
    case SCH_FIELD_T:
    case SCH_SHEET_T:
    case SCH_BITMAP_T:
        PrepareMoveItem( item );
        break;

    case SCH_MARKER_T:
        // Moving a marker has no sense
        break;

    default:
        // Unknown items cannot be moved
        wxFAIL_MSG( wxString::Format( "Cannot move item type %d", item->Type() ) );
        break;
    }

    if( GetToolId() == ID_NO_TOOL_SELECTED )
        SetRepeatItem( NULL );
}


void SCH_EDIT_FRAME::OnCancelCurrentCommand( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();

    if( screen->IsBlockActive() )
    {
        GetCanvas()->SetCursor( (wxStockCursor) GetGalCanvas()->GetDefaultCursor() );
        screen->ClearBlockCommand();

        // Stop the current command (if any) but keep the current tool
        m_canvas->EndMouseCapture();
    }
    else
    {
        if( m_canvas->IsMouseCaptured() ) // Stop the current command but keep the current tool
            m_canvas->EndMouseCapture();
        else                              // Deselect current tool
            m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, GetGalCanvas()->GetDefaultCursor() );
     }

     GetCanvas()->GetView()->ClearHiddenFlags();
     GetCanvas()->GetView()->ClearPreview();
     GetCanvas()->GetView()->ShowPreview( false );
}


void SCH_EDIT_FRAME::OnUpdateSelectTool( wxUpdateUIEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_drawToolBar || aEvent.GetEventObject() == m_mainToolBar )
        aEvent.Check( GetToolId() == aEvent.GetId() );
}


void SCH_EDIT_FRAME::DeleteConnection( bool aFullConnection )
{
    PICKED_ITEMS_LIST   pickList;
    SCH_SCREEN*         screen = GetScreen();
    wxPoint             pos = GetCrossHairPosition();

    if( screen->GetConnection( pos, pickList, aFullConnection ) != 0 )
    {
        DeleteItemsInList( pickList );
        SchematicCleanUp( true );
        OnModify();
    }
}


// This function is a callback function, called by the mouse cursor moving event
static void moveItemWithMouseCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                     const wxPoint& aPosition, bool aErase )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM*   item   = screen->GetCurItem();
    auto panel = static_cast<SCH_DRAW_PANEL*>( aPanel );
    auto view = panel->GetView();

    wxCHECK_RET( (item != NULL), wxT( "Cannot move invalid schematic item." ) );

    wxPoint cpos = aPanel->GetParent()->GetCrossHairPosition();
    cpos -= item->GetStoredPos();

    item->SetPosition( cpos );

    view->Hide( item );
    view->ClearPreview();
    view->AddToPreview( item->Clone() );

    // Needed when moving a bitmap image to avoid ugly rendering and artifacts,
    // because a bitmap is drawn only as non cached
    if( item->Type() == SCH_BITMAP_T )
        view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}


/**
 * Callback function called when aborting a move item with mouse cursor command.
 */
static void abortMoveItem( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN*     screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM*       item = screen->GetCurItem();
    SCH_EDIT_FRAME* parent = (SCH_EDIT_FRAME*) aPanel->GetParent();
    auto panel = static_cast<SCH_DRAW_PANEL*>( aPanel );
    auto view = panel->GetView();

    parent->SetRepeatItem( NULL );
    screen->SetCurItem( NULL );
    view->ClearPreview();
    view->ClearHiddenFlags();

    if( item == NULL )  /* no current item */
        return;

    if( item->IsNew() )
    {
        delete item;
    }
    else
    {
        SCH_ITEM* oldItem = parent->GetUndoItem();

        SCH_ITEM* currentItem;

        // Items that are children of other objects are undone by swapping the contents
        // of the parent items.
        if( (item->Type() == SCH_SHEET_PIN_T) || (item->Type() == SCH_FIELD_T) )
            currentItem = (SCH_ITEM*) item->GetParent();
        else
            currentItem = item;

        wxCHECK_RET( oldItem != NULL && currentItem->Type() == oldItem->Type(),
                     wxT( "Cannot restore undefined or bad last schematic item." ) );

        // Never delete existing item, because it can be referenced by an undo/redo command
        // Just restore its data

        view->Remove( currentItem );
        currentItem->SwapData( oldItem );
        item->ClearFlags();
        view->Add( currentItem );
        view->Hide( item, false );
    }

    screen->TestDanglingEnds();
    aPanel->Refresh();
}


void SCH_EDIT_FRAME::PrepareMoveItem( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot move invalid schematic item" ) );

    GetToolManager()->DeactivateTool();

    SetRepeatItem( NULL );

    if( !aItem->IsNew() )
    {
        if( (aItem->Type() == SCH_SHEET_PIN_T) || (aItem->Type() == SCH_FIELD_T) )
            SetUndoItem( (SCH_ITEM*) aItem->GetParent() );
        else
            SetUndoItem( aItem );
    }

    std::vector<DANGLING_END_ITEM> emptySet;
    aItem->UpdateDanglingState( emptySet );

    aItem->SetFlags( IS_MOVED );

    if( aItem->Type() == SCH_FIELD_T && aItem->GetParent()->Type() == SCH_COMPONENT_T )
    {
        // Now that we're moving a field, they're no longer autoplaced.
        SCH_COMPONENT *parent = static_cast<SCH_COMPONENT*>( aItem->GetParent() );
        parent->ClearFieldsAutoplaced();
    }

    // These are owned by their parent, and so their parent must erase them
    if( aItem->Type() == SCH_SHEET_PIN_T || aItem->Type() == SCH_FIELD_T )
        RefreshItem( aItem );

    // For some items, moving the cursor to anchor is not good (for instance large
    // hierarchical sheets or components can have the anchor outside the view)
    if( aItem->IsMovableFromAnchorPoint() )
    {
        SetCrossHairPosition( aItem->GetPosition() );
        m_canvas->MoveCursorToCrossHair();
        aItem->SetStoredPos( wxPoint( 0,0 ) );
    }
    else
    {
        // Round the point under the cursor to a multiple of the grid
        wxPoint cursorpos = GetCrossHairPosition() - aItem->GetPosition();
        wxPoint gridsize = GetScreen()->GetGridSize();
        cursorpos.x = ( cursorpos.x / gridsize.x ) * gridsize.x;
        cursorpos.y = ( cursorpos.y / gridsize.y ) * gridsize.y;

        aItem->SetStoredPos( cursorpos );
    }

    if( !aItem->IsNew() )
        OnModify();

    GetScreen()->SetCurItem( aItem );
    m_canvas->SetMouseCapture( moveItemWithMouseCursor, abortMoveItem );
    m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );

    m_canvas->Refresh();
}


void SCH_EDIT_FRAME::SelectAllFromSheet( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SCREEN*         screen = GetScreen();
    SCH_ITEM*           item = screen->GetCurItem();

    if( item != NULL )
    {
        item = selTool->SelectPoint( item->GetPosition() );
        SendMessageToPCBNEW( item, NULL );
    }
    else
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = selTool->SelectPoint( data->GetPosition() );
        SendMessageToPCBNEW( item, NULL );
    }
}


void SCH_EDIT_FRAME::OnRotate( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SCREEN*         screen = GetScreen();
    SCH_ITEM*           item = screen->GetCurItem();
    BLOCK_SELECTOR&     block = screen->m_BlockLocate;

    // Allows block rotate operation on hot key.
    if( block.GetState() != STATE_NO_BLOCK )
    {
        // Compute the rotation center and put it on grid:
        wxPoint rotationPoint = GetNearestGridPosition( block.Centre() );

        if( block.GetCommand() != BLOCK_DUPLICATE && block.GetCommand() != BLOCK_PASTE )
        {
            SaveCopyInUndoList( block.GetItems(), UR_ROTATED, block.AppendUndo(), rotationPoint );
            block.SetAppendUndo();
        }

        RotateListOfItems( block.GetItems(), rotationPoint );

        m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
        m_canvas->Refresh();
        return;
    }

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = selTool->SelectPoint( data->GetPosition(), SCH_COLLECTOR::RotatableItems );

        // Exit if no item found at the current location or the item is already being edited.
        if( item == NULL || item->GetEditFlags() != 0 )
            return;
    }

    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

            if( aEvent.GetId() == ID_SCH_ROTATE_CLOCKWISE )
                OrientComponent( CMP_ROTATE_CLOCKWISE );
            else if( aEvent.GetId() == ID_SCH_ROTATE_COUNTERCLOCKWISE )
                OrientComponent( CMP_ROTATE_COUNTERCLOCKWISE );
            else
                wxFAIL_MSG( wxT( "Unknown rotate item command ID." ) );

            if( m_autoplaceFields )
                component->AutoAutoplaceFields( GetScreen() );

            break;
        }

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
        m_canvas->MoveCursorToCrossHair();
        ChangeTextOrient( (SCH_TEXT*) item );
        break;

    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
        m_canvas->MoveCursorToCrossHair();
        SaveCopyInUndoList( item, UR_CHANGED );
        item->Rotate( m_canvas->GetParent()->GetCrossHairPosition() );
        break;

    case SCH_FIELD_T:
        m_canvas->MoveCursorToCrossHair();
        RotateField( (SCH_FIELD*) item );
        if( item->GetParent()->Type() == SCH_COMPONENT_T )
        {
            // Now that we're moving a field, they're no longer autoplaced.
            SCH_COMPONENT *parent = static_cast<SCH_COMPONENT*>( item->GetParent() );
            parent->ClearFieldsAutoplaced();
        }
        break;

    case SCH_BITMAP_T:
        RotateImage( (SCH_BITMAP*) item );
        // The bitmap is cached in Opengl: clear the cache, because
        // the cache data is invalid
        GetCanvas()->GetView()->RecacheAllItems();
        break;

    case SCH_SHEET_T:
        if( !item->IsNew() )    // rotate a sheet during its creation has no sense
        {
            bool retCCW = ( aEvent.GetId() == ID_SCH_ROTATE_COUNTERCLOCKWISE );
            RotateHierarchicalSheet( static_cast<SCH_SHEET*>( item ), retCCW );
        }

        break;

    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
        // these items are not rotated, because rotation does not change them.
        break;

    default:
        // Other items (wires...) cannot be rotated, at least during creation
        if( item->IsNew() )
            break;

        wxFAIL_MSG( wxString::Format( wxT( "Cannot rotate schematic item type %s." ),
                                      GetChars( item->GetClass() ) ) );
    }

    RefreshItem( item );

    if( item->IsMoving() )
    {
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
        else
            m_toolManager->RunAction( SCH_ACTIONS::refreshPreview, true );
    }

    if( item->GetEditFlags() == 0 )
        screen->SetCurItem( nullptr );
}


void SCH_EDIT_FRAME::OnEditItem( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SCREEN*         screen = GetScreen();
    SCH_ITEM*           item = screen->GetCurItem();

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        // Set the locat filter, according to the edit command
        const KICAD_T* filterList = SCH_COLLECTOR::EditableItems;
        const KICAD_T* filterListAux = NULL;

        switch( aEvent.GetId() )
        {
        case ID_SCH_EDIT_COMPONENT_REFERENCE:
            filterList = SCH_COLLECTOR::CmpFieldReferenceOnly;
            filterListAux = SCH_COLLECTOR::ComponentsOnly;
            break;

        case ID_SCH_EDIT_COMPONENT_VALUE:
            filterList = SCH_COLLECTOR::CmpFieldValueOnly;
            filterListAux = SCH_COLLECTOR::ComponentsOnly;
            break;

        case ID_SCH_EDIT_COMPONENT_FOOTPRINT:
            filterList = SCH_COLLECTOR::CmpFieldFootprintOnly;
            filterListAux = SCH_COLLECTOR::ComponentsOnly;
            break;

        case ID_POPUP_SCH_DISPLAYDOC_CMP:
            filterList = SCH_COLLECTOR::CmpFieldDatasheetOnly;
            filterListAux = SCH_COLLECTOR::ComponentsOnly;

        default:
            break;
        }

        item = selTool->SelectPoint( data->GetPosition(), filterList );

        // If no item found, and if an auxiliary filter exists, try to use it
        if( !item && filterListAux )
            item = selTool->SelectPoint( data->GetPosition(), filterListAux );

        // Exit if no item found at the current location or the item is already being edited.
        if( item == NULL || item->GetEditFlags() != 0 )
            return;
    }

    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
    {
        switch( aEvent.GetId() )
        {
        case ID_SCH_EDIT_COMPONENT_REFERENCE:
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( REFERENCE ) );
            break;

        case ID_SCH_EDIT_COMPONENT_VALUE:
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( VALUE ) );
            break;

        case ID_SCH_EDIT_COMPONENT_FOOTPRINT:
            EditComponentFieldText( ( (SCH_COMPONENT*) item )->GetField( FOOTPRINT ) );
            break;

        case ID_POPUP_SCH_DISPLAYDOC_CMP:
        {
            wxString text = static_cast<SCH_COMPONENT*>( item )->GetField( DATASHEET )->GetText();

            if( !text.IsEmpty() )
                GetAssociatedDocument( this, text );
        }
            break;

        case ID_SCH_EDIT_ITEM:
            EditComponent( (SCH_COMPONENT*) item );
            break;

        default:
            wxFAIL_MSG( wxString::Format( wxT( "Invalid schematic component edit command ID %d" ),
                                          aEvent.GetId() ) );
        }

        break;
    }

    case SCH_SHEET_T:
        {
        bool doClearAnnotation;
        bool doRefresh = false;
        // Keep trace of existing sheet paths. EditSheet() can modify this list
        SCH_SHEET_LIST initial_sheetpathList( g_RootSheet );

        doRefresh = EditSheet( (SCH_SHEET*) item, g_CurrentSheet, &doClearAnnotation );

        if( doClearAnnotation )     // happens when the current sheet load a existing file
        {                           // we must clear "new" components annotation
            SCH_SCREENS screensList( g_RootSheet );
            // We clear annotation of new sheet paths here:
            screensList.ClearAnnotationOfNewSheetPaths( initial_sheetpathList );
            // Clear annotation of g_CurrentSheet itself, because its sheetpath
            // is not a new path, but components managed by its sheet path must have
            // their annotation cleared, becuase they are new:
            ((SCH_SHEET*) item)->GetScreen()->ClearAnnotation( g_CurrentSheet );
        }

        if( doRefresh )
            m_canvas->Refresh();
        }
        break;

    case SCH_SHEET_PIN_T:
        EditSheetPin( (SCH_SHEET_PIN*) item, true );
        break;

    case SCH_TEXT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
        EditSchematicText( (SCH_TEXT*) item );
        break;

    case SCH_FIELD_T:
        EditComponentFieldText( (SCH_FIELD*) item );
        break;

    case SCH_BITMAP_T:

        // The bitmap is cached in Opengl: clear the cache, because
        // the cache data is perhaps invalid
        if( EditImage( (SCH_BITMAP*) item ) )
            GetCanvas()->GetView()->RecacheAllItems();

        break;

    case SCH_LINE_T:        // These items have no param to edit
        EditLine( (SCH_LINE*) item, true );
        break;
    case SCH_MARKER_T:
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
        break;

    default:                // Unexpected item
        wxFAIL_MSG( wxString( "Cannot edit schematic item type " ) + item->GetClass() );
    }

    RefreshItem( item );

    if( item->GetEditFlags() == 0 )
        screen->SetCurItem( nullptr );
}


void SCH_EDIT_FRAME::OnDragItem( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SCREEN*         screen = GetScreen();
    SCH_ITEM*           item = screen->GetCurItem();

    // The easiest way to handle a menu or a hot key drag command
    // is to simulate a block drag command
    //
    // When a drag item is requested, some items use a BLOCK_DRAG_ITEM drag type
    // an some items use a BLOCK_DRAG drag type  (mainly a junction)
    // a BLOCK_DRAG collects all items in a block (here a 2x2 rect centered on the cursor)
    // and BLOCK_DRAG_ITEM drag only the selected item
    BLOCK_COMMAND_T dragType = BLOCK_DRAG_ITEM;

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = selTool->SelectPoint( data->GetPosition(), SCH_COLLECTOR::DraggableItems );

        // Exit if no item found at the current location or the item is already being edited.
        if( item == NULL || item->GetEditFlags() != 0 )
            return;

        // When a junction or a node is found, a BLOCK_DRAG is better
        if( m_collectedItems.IsCorner() || m_collectedItems.IsNode( false )
            || m_collectedItems.IsDraggableJunction() )
        {
            dragType = BLOCK_DRAG;
        }
    }

    switch( item->Type() )
    {
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_LINE_T:
    case SCH_JUNCTION_T:
    case SCH_COMPONENT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_SHEET_T:
    case SCH_TEXT_T:
        m_canvas->MoveCursorToCrossHair();

        if( screen->m_BlockLocate.GetState() == STATE_NO_BLOCK )
        {
            if( !HandleBlockBegin( nullptr, dragType, GetCrossHairPosition() ) )
                break;

            // Give a non null size to the search block:
            screen->m_BlockLocate.Inflate( 1 );
            screen->m_BlockLocate.SetLastCursorPosition( GetCrossHairPosition() );
            HandleBlockEnd( nullptr );
        }

        break;

    default:
        wxFAIL_MSG( wxString( "Cannot drag schematic item type " ) + item->GetClass() );
    }
}


void SCH_EDIT_FRAME::OnOrient( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL* selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    SCH_SCREEN*         screen  = GetScreen();
    SCH_ITEM*           item    = screen->GetCurItem();
    BLOCK_SELECTOR&     block   = screen->m_BlockLocate;

    // Allows block rotate operation on hot key.
    if( block.GetState() != STATE_NO_BLOCK )
    {
        if( aEvent.GetId() == ID_SCH_MIRROR_X )
        {
            // Compute the mirror center and put it on grid.
            wxPoint mirrorPt = block.Centre();
            mirrorPt = GetNearestGridPosition( mirrorPt );
            SetCrossHairPosition( mirrorPt );

            if( block.GetCommand() != BLOCK_DUPLICATE && block.GetCommand() != BLOCK_PASTE )
            {
                SaveCopyInUndoList( block.GetItems(), UR_MIRRORED_X, block.AppendUndo(), mirrorPt );
                block.SetAppendUndo();
            }

            MirrorX( block.GetItems(), mirrorPt );

            m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
            m_canvas->Refresh();
            return;
        }
        else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
        {
            // Compute the mirror center and put it on grid.
            wxPoint mirrorPt = block.Centre();
            mirrorPt = GetNearestGridPosition( mirrorPt );
            SetCrossHairPosition( mirrorPt );

            if( block.GetCommand() != BLOCK_DUPLICATE && block.GetCommand() != BLOCK_PASTE )
            {
                SaveCopyInUndoList( block.GetItems(), UR_MIRRORED_Y, block.AppendUndo(), mirrorPt );
                block.SetAppendUndo();
            }

            MirrorY( block.GetItems(), mirrorPt );

            m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
            m_canvas->Refresh();
            return;
        }
        else
        {
            wxFAIL_MSG( wxT( "Unknown block oriention command ID." ) );
        }

        return;
    }

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();

        wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

        item = selTool->SelectPoint( data->GetPosition(), SCH_COLLECTOR::OrientableItems );

        // Exit if no item found at the current location or the item is already being edited.
        if( item == NULL || item->GetEditFlags() != 0 )
            return;
    }


    switch( item->Type() )
    {
    case SCH_COMPONENT_T:
        {
            SCH_COMPONENT *component = static_cast<SCH_COMPONENT*>( item );
            if( aEvent.GetId() == ID_SCH_MIRROR_X )
                OrientComponent( CMP_MIRROR_X );
            else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
                OrientComponent( CMP_MIRROR_Y );
            else if( aEvent.GetId() == ID_SCH_ORIENT_NORMAL )
                OrientComponent( CMP_NORMAL );
            else
                wxFAIL_MSG( wxT( "Invalid orient schematic component command ID." ) );

            if( m_autoplaceFields )
                component->AutoAutoplaceFields( GetScreen() );

            break;
        }

    case SCH_BITMAP_T:
        if( aEvent.GetId() == ID_SCH_MIRROR_X )
            MirrorImage( (SCH_BITMAP*) item, true );
        else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
            MirrorImage( (SCH_BITMAP*) item, false );

        // The bitmap is cached in Opengl: clear the cache, because
        // the cache data is invalid
        GetCanvas()->GetView()->RecacheAllItems();
        break;

    case SCH_SHEET_T:
        if( aEvent.GetId() == ID_SCH_MIRROR_X )
            MirrorSheet( (SCH_SHEET*) item, true );
        else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
            MirrorSheet( (SCH_SHEET*) item, false );

        break;

    case SCH_BUS_BUS_ENTRY_T:
    case SCH_BUS_WIRE_ENTRY_T:
        m_canvas->MoveCursorToCrossHair();
        SaveCopyInUndoList( item, UR_CHANGED );

        if( aEvent.GetId() == ID_SCH_MIRROR_X )
            item->MirrorX( m_canvas->GetParent()->GetCrossHairPosition().y );
        else if( aEvent.GetId() == ID_SCH_MIRROR_Y )
            item->MirrorY( m_canvas->GetParent()->GetCrossHairPosition().x );

        break;

    default:
        // This object cannot be oriented.
        ;
    }

    RefreshItem( item );

    if( item->IsMoving() )
    {
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( nullptr, wxDefaultPosition, false );
        else
            m_toolManager->RunAction( SCH_ACTIONS::refreshPreview, true );
    }

    if( item->GetEditFlags() == 0 )
        screen->SetCurItem( nullptr );
}


void SCH_EDIT_FRAME::OnUnfoldBusHotkey( wxCommandEvent& aEvent )
{
    SCH_SELECTION_TOOL*     selTool = GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
    EDA_HOTKEY_CLIENT_DATA* data = (EDA_HOTKEY_CLIENT_DATA*) aEvent.GetClientObject();
    SCH_ITEM*               item = GetScreen()->GetCurItem();

    wxCHECK_RET( data != NULL, wxT( "Invalid hot key client object." ) );

    if( item == NULL )
    {
        // If we didn't get here by a hot key, then something has gone wrong.
        if( aEvent.GetInt() == 0 )
            return;

        item = selTool->SelectPoint( data->GetPosition(), SCH_COLLECTOR::EditableItems );

        // Exit if no item found at the current location or the item is already being edited.
        if( item == NULL || item->GetEditFlags() != 0 )
            return;
    }

    auto connection = item->Connection( *g_CurrentSheet );

    if( connection && connection->IsBus() )
    {
        int idx = 0;
        wxMenu* bus_unfolding_menu = new wxMenu;

        for( const auto& member : connection->Members() )
        {
            int id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );

            if( member->Type() == CONNECTION_BUS )
            {
                wxMenu* submenu = new wxMenu;
                bus_unfolding_menu->AppendSubMenu( submenu, _( member->Name() ) );

                for( const auto& sub_member : member->Members() )
                {
                    id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );

                    submenu->Append( id, sub_member->Name(), wxEmptyString );

                    // See comment in else clause below
                    auto sub_item_clone = new wxMenuItem();
                    sub_item_clone->SetItemLabel( sub_member->Name() );

                    Bind( wxEVT_COMMAND_MENU_SELECTED, &SCH_EDIT_FRAME::OnUnfoldBus,
                                 this, id, id, sub_item_clone );
                }
            }
            else
            {
                bus_unfolding_menu->Append( id, member->Name(), wxEmptyString );

                // Because Bind() takes ownership of the user data item, we
                // make a new menu item here and set its label.  Why create a
                // menu item instead of just a wxString or something? Because
                // Bind() requires a pointer to wxObject rather than a void
                // pointer.  Maybe at some point I'll think of a better way...
                auto item_clone = new wxMenuItem();
                item_clone->SetItemLabel( member->Name() );

                Bind( wxEVT_COMMAND_MENU_SELECTED, &SCH_EDIT_FRAME::OnUnfoldBus,
                             this, id, id, item_clone );
            }
        }

        auto controls = GetCanvas()->GetViewControls();
        auto vmp = controls->GetMousePosition( false );
        wxPoint mouse_pos( (int) vmp.x, (int) vmp.y );

        GetGalCanvas()->PopupMenu( bus_unfolding_menu, mouse_pos );
    }
}

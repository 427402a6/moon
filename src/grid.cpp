/*
 * grid.cpp: canvas definitions.
 *
 * Contact:
 *   Moonlight List (moonlight-list@lists.ximian.com)
 *
 * Copyright 2008 Novell, Inc. (http://www.novell.com)
 *
 * See the LICENSE file included with the distribution for details.
 * 
 */

#include <config.h>

#include <math.h>

#include "brush.h"
#include "rect.h"
#include "canvas.h"
#include "grid.h"
#include "runtime.h"
#include "namescope.h"
#include "collection.h"

Grid::Grid ()
{
	SetObjectType (Type::GRID);
}

Grid::~Grid ()
{
}

void
Grid::OnPropertyChanged (PropertyChangedEventArgs *args)
{
	if (args->property->GetOwnerType() != Type::GRID) {
		Panel::OnPropertyChanged (args);
		return;
	}

	if (args->property == Grid::ShowGridLinesProperty){
		Invalidate ();
	}

	InvalidateMeasure ();

	NotifyListenersOfPropertyChange (args);
}

void
Grid::OnSubPropertyChanged (DependencyProperty *prop, DependencyObject *obj, PropertyChangedEventArgs *subobj_args)
{
	if (subobj_args->property == Grid::ColumnProperty
	    || subobj_args->property == Grid::RowProperty
	    || subobj_args->property == Grid::ColumnSpanProperty
	    || subobj_args->property == Grid::RowSpanProperty) {
		InvalidateMeasure ();
	}
	
	Panel::OnSubPropertyChanged (prop, obj, subobj_args);
}

void
Grid::OnCollectionChanged (Collection *col, CollectionChangedEventArgs *args)
{
	if (col == GetColumnDefinitions () ||
	    col == GetRowDefinitions ()) {

	} else {
		Panel::OnCollectionChanged (col, args);
	}
	
	InvalidateMeasure ();
}

Size
Grid::MeasureOverride (Size availableSize)
{
	Size results = availableSize;

	ColumnDefinitionCollection *columns = GetColumnDefinitions ();
	RowDefinitionCollection *rows = GetRowDefinitions ();
	bool free_col = false;
	bool free_row = false;

	int col_count = columns->GetCount ();
	int row_count = rows->GetCount ();

	if (col_count == 0) {
		columns = new ColumnDefinitionCollection ();
		ColumnDefinition *coldef = new ColumnDefinition ();
		columns->Add (coldef);
		coldef->unref ();
		free_col = true;
		col_count = 1;
	}

	if (row_count == 0) {
		rows = new RowDefinitionCollection ();
		RowDefinition *rowdef = new RowDefinition ();
		rows->Add (rowdef);
		rowdef->unref ();
		free_row = true;
		row_count = 1;
	}

	for (int i = 0; i < row_count; i ++) {
		RowDefinition *rowdef = rows->GetValueAt (i)->AsRowDefinition ();
		GridLength* height = rowdef->GetHeight();

		rowdef->SetActualHeight (0.0);

		if (height->type == GridUnitTypePixel)
                       rowdef->SetActualHeight (height->val);
	}

	for (int i = 0; i < col_count; i ++) {
		ColumnDefinition *coldef = columns->GetValueAt (i)->AsColumnDefinition ();
		GridLength *width = coldef->GetWidth ();

		coldef->SetActualWidth (0.0);

		if (width->type == GridUnitTypePixel)
			coldef->SetActualWidth (width->val);

	}
	
	magic = Size ();
	VisualTreeWalker walker = VisualTreeWalker (this);
	while (UIElement *child = walker.Step ()) {
		if (child->GetVisibility () != VisibilityVisible)
			continue;

		gint32 col, row;
		gint32 colspan, rowspan;

		col = MIN (Grid::GetColumn (child), col_count - 1);
		row = MIN (Grid::GetRow (child), row_count - 1);
		colspan = MIN (Grid::GetColumnSpan (child), col_count - col);
		rowspan = MIN (Grid::GetRowSpan (child), row_count - row);
		Size pixels = Size ();
		Size stars = Size ();
		Size automatic = Size ();


		Size child_size = Size (0,0);
		Size min_size = Size (0,0);
		Size max_size = Size (0,0); 
		
		for (int r = row; r < row + rowspan; r++) {
			RowDefinition *rowdef = rows->GetValueAt (r)->AsRowDefinition ();
			GridLength* height = rowdef->GetHeight();

			switch (height->type) {
			case GridUnitTypePixel:
				pixels.height += height->val;
				child_size.height += height->val;
				break;
			case GridUnitTypeAuto:
				automatic.height += 1;
				child_size.height += rowdef->GetMaxHeight ();
				break;
			case GridUnitTypeStar:
				stars.height += height->val;
				child_size.height += rowdef->GetMaxHeight ();				
				break;
			}

			min_size.height += rowdef->GetMinHeight ();
			max_size.height += rowdef->GetMaxHeight ();
		}

		for (int c = col; c < col + colspan; c++) {
			ColumnDefinition *coldef = columns->GetValueAt (c)->AsColumnDefinition ();
			GridLength* width = coldef->GetWidth();
			
			switch (width->type) {
			case GridUnitTypePixel:
				pixels.width += width->val;
				child_size.width += width->val;
				break;
			case GridUnitTypeAuto:
				automatic.width += 1;
				child_size.width += coldef->GetMaxWidth ();
				break;
			case GridUnitTypeStar:
				stars.width += width->val;
				child_size.width += coldef->GetMaxWidth ();
				break;
			}

			min_size.width += coldef->GetMinWidth ();
			max_size.width += coldef->GetMaxWidth ();
		}
		
		
		child_size = child_size.Min (max_size);
		child_size = child_size.Max (min_size);

		child->Measure (child_size);
		Size desired = child->GetDesiredSize();
		
		Size remaining = desired;
		for (int c = col; c < col + colspan; c++){
			ColumnDefinition *coldef = columns->GetValueAt (c)->AsColumnDefinition ();
			GridLength *width = coldef->GetWidth ();

			double contribution = 0;
			switch (width->type) {
			case GridUnitTypeAuto:
				if (stars.width <= 0)
					contribution = (desired.width - pixels.width) / automatic.width;
				break;
			case GridUnitTypePixel:
				contribution = width->val;
				break;
			default:
				contribution = remaining.width;
				break;
			}
			
			contribution = MAX (contribution, coldef->GetMinWidth ());
			contribution = MIN (contribution, coldef->GetMaxWidth ());
			
			coldef->SetActualWidth (MAX(coldef->GetActualWidth (), contribution));
			remaining.width -= contribution; 
		}

		for (int r = row; r < row + rowspan; r++){
			RowDefinition *rowdef = rows->GetValueAt (r)->AsRowDefinition ();
			GridLength *height = rowdef->GetHeight ();

			double contribution = 0;
			switch (height->type) {
			case GridUnitTypeAuto:
				if (stars.height <= 0)
					contribution = (desired.height - pixels.height) / automatic.height;
				break;
			case GridUnitTypePixel:
				contribution = height->val;
				break;
			default:
				contribution = remaining.height;
				break;
			}

			contribution = MAX (contribution, rowdef->GetMinHeight ());
			contribution = MIN (contribution, rowdef->GetMaxHeight ());
			
			rowdef->SetActualHeight (MAX (rowdef->GetActualHeight (), contribution));
			remaining.height -= contribution;
		}
	}

	Size grid_size;
	for (int r = 0; r < row_count; r ++) {
		grid_size.height += rows->GetValueAt (r)->AsRowDefinition ()->GetActualHeight ();
	}

	for (int c = 0; c < col_count; c ++) {
		grid_size.width += columns->GetValueAt (c)->AsColumnDefinition ()->GetActualWidth ();
	}

	grid_size = grid_size.Max (GetWidth (), GetHeight ());
	results = results.Min (grid_size);

	if (free_col) {
		magic.width = columns->GetValueAt (0)->AsColumnDefinition ()->GetActualWidth ();
		columns->unref ();
	}

	if (free_row) {
		magic.height = rows->GetValueAt (0)->AsRowDefinition ()->GetActualHeight ();
		rows->unref ();
	}
	// now choose whichever is smaller, our chosen size or the availableSize.
	return results;
}

Size
Grid::ArrangeOverride (Size finalSize)
{
	ColumnDefinitionCollection *columns = GetColumnDefinitions ();
	RowDefinitionCollection *rows = GetRowDefinitions ();
	bool free_col = false;
	bool free_row = false;

	int col_count = columns->GetCount ();
	int row_count = rows->GetCount ();

	if (col_count == 0) {
		columns = new ColumnDefinitionCollection ();
		ColumnDefinition *coldef = new ColumnDefinition ();
		coldef->SetActualWidth (magic.width);
		columns->Add (coldef);
		coldef->unref ();
		free_col = true;
		col_count = 1;
	}

	if (row_count == 0) {
		rows = new RowDefinitionCollection ();
		RowDefinition *rowdef = new RowDefinition ();
		rowdef->SetActualHeight (magic.height);
		rows->Add (rowdef);
		rowdef->unref ();
		free_row = true;
		row_count = 1;
	}

	Size requested = Size ();
	Size star_size = Size ();
	double row_stars = 0.0;
	HorizontalAlignment horiz = !isnan (GetWidth ()) ? HorizontalAlignmentStretch : GetHorizontalAlignment ();
	VerticalAlignment vert = !isnan (GetHeight ()) ? VerticalAlignmentStretch : GetVerticalAlignment ();

	for (int i = 0; i < row_count; i ++) {
		RowDefinition *rowdef = rows->GetValueAt (i)->AsRowDefinition ();
		GridLength* height = rowdef->GetHeight();

		switch (height->type) {
		case GridUnitTypeStar:
			// Star columns distribute evenly
			requested.height += rowdef->GetActualHeight ();
			star_size.height += rowdef->GetActualHeight ();
			//if (vert == VerticalAlignmentStretch)
			//	rowdef->SetActualHeight (0.0);

			row_stars += height->val;
			break;
		case GridUnitTypePixel:
			requested.height += rowdef->GetActualHeight ();
			break;
		case GridUnitTypeAuto:
			requested.height += rowdef->GetActualHeight ();
			break;
		}
	}

	double col_stars = 0.0;
	for (int i = 0; i < col_count; i ++) {
		ColumnDefinition *coldef = columns->GetValueAt (i)->AsColumnDefinition ();
		GridLength* width = coldef->GetWidth();

		switch (width->type) {
		case GridUnitTypeStar:
			// Star columns distribute evenly
			requested.width += coldef->GetActualWidth ();
			star_size.width += coldef->GetActualWidth ();
			//if (horiz == HorizontalAlignmentStretch)
			//	coldef->SetActualWidth (0.0);

			col_stars += width->val;
			break;
		case GridUnitTypePixel:
			requested.width += coldef->GetActualWidth ();
			break;
		case GridUnitTypeAuto:
			requested.width += coldef->GetActualWidth ();
			break;
		}
	}

	Size remaining = Size (finalSize.width - requested.width, finalSize.height - requested.height);

	if (horiz != HorizontalAlignmentStretch)
		remaining.width = MIN (remaining.width, 0);

	if (vert != VerticalAlignmentStretch)
		remaining.height = MIN (remaining.height, 0);

	if (remaining.height != 0) {     
		remaining.height += star_size.height;

		for (int i = 0; i < row_count; i ++) {
			RowDefinition *rowdef = rows->GetValueAt (i)->AsRowDefinition ();
			GridLength* height = rowdef->GetHeight();
			
			if (height->type == GridUnitTypeStar)
				rowdef->SetActualHeight (MAX ((remaining.height * height->val / row_stars), 0));
		}
	}
	
	if (remaining.width != 0) {
		remaining.width += star_size.width;

		for (int i = 0; i < col_count; i ++) {
			ColumnDefinition *coldef = columns->GetValueAt (i)->AsColumnDefinition ();
			GridLength* width = coldef->GetWidth();
			
			if (width->type == GridUnitTypeStar)
				coldef->SetActualWidth (MAX ((remaining.width * width->val / col_stars), 0));
		}
       	}

	bool first = true;
	Size arranged = finalSize;
	
	VisualTreeWalker walker = VisualTreeWalker (this);
	while (UIElement *child = walker.Step ()) {
		if (child->GetVisibility () != VisibilityVisible)
			continue;

		gint32 col = MIN (Grid::GetColumn (child), col_count - 1);
		gint32 row = MIN (Grid::GetRow (child), row_count - 1);
		gint32 colspan = MIN (Grid::GetColumnSpan (child), col_count - col);
		gint32 rowspan = MIN (Grid::GetRowSpan (child), row_count - row);

		Rect child_final = Rect (0, 0, 0, 0);
		Size min_size;
		Size max_size;

		if (first) {
			arranged = Size ();
			first = false;
		}

		for (int r = 0; r < row + rowspan; r++) {
			RowDefinition *rowdef = rows->GetValueAt (r)->AsRowDefinition ();

			if (r < row) {
				child_final.y += rowdef->GetActualHeight ();
			} else {
				child_final.height += rowdef->GetActualHeight ();
				
				min_size.height += rowdef->GetMinHeight ();
				max_size.height += rowdef->GetMaxHeight ();
			}
		}

		for (int c = 0; c < col + colspan; c++) {
			ColumnDefinition *coldef = columns->GetValueAt (c)->AsColumnDefinition ();

			if (c < col) {
				child_final.x += coldef->GetActualWidth ();
			} else {
				child_final.width += coldef->GetActualWidth ();

				min_size.width += coldef->GetMinWidth ();
				max_size.width += coldef->GetMaxWidth ();
			}
		}

		child->Arrange (child_final);
		Size child_arranged = child->GetRenderSize ();

		if (horiz == HorizontalAlignmentStretch)
			arranged.width = MAX (child_final.x + child_final.width, finalSize.width);
		else 
			arranged.width = MAX (child_final.x + child_final.width, arranged.width);
		    
		if (vert == VerticalAlignmentStretch)
			arranged.height = MAX (child_final.y + child_final.height, finalSize.height);
		else
			arranged.height = MAX (child_final.y + child_final.height, arranged.height);
	}

	if (free_col)
		columns->unref ();

	if (free_row)
		rows->unref ();

	return arranged;
}

//
// ColumnDefinitionCollection
//

ColumnDefinitionCollection::ColumnDefinitionCollection ()
{
	SetObjectType (Type::COLUMNDEFINITION_COLLECTION);
}

ColumnDefinitionCollection::~ColumnDefinitionCollection ()
{
}


//
// ColumnDefinition
//

ColumnDefinition::ColumnDefinition ()
  : actual (0.0)
{
	SetObjectType (Type::COLUMNDEFINITION);
}

ColumnDefinition::~ColumnDefinition ()
{
}

//
// RowDefinitionCollection
//

RowDefinitionCollection::RowDefinitionCollection ()
{
	SetObjectType (Type::ROWDEFINITION_COLLECTION);
}

RowDefinitionCollection::~RowDefinitionCollection ()
{
}

//
// RowDefinition
//

RowDefinition::RowDefinition ()
  : actual (0.0)
{
	SetObjectType (Type::ROWDEFINITION);
}

RowDefinition::~RowDefinition ()
{
}

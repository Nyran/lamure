#include <fstream>
#include <string>

#include "lamure/pvs/pvs_database.h"
#include "lamure/pvs/grid_regular.h"
#include "lamure/pvs/grid_regular_compressed.h"
#include "lamure/pvs/grid_octree.h"
#include "lamure/pvs/grid_octree_hierarchical.h"
#include "lamure/pvs/grid_octree_hierarchical_v2.h"
#include "lamure/pvs/grid_octree_hierarchical_v3.h"
#include "lamure/pvs/grid_irregular.h"

namespace lamure
{
namespace pvs
{

pvs_database* pvs_database::instance_ = nullptr;

pvs_database::
pvs_database()
{
	visibility_grid_ = nullptr;
	viewer_cell_ = nullptr;
	activated_ = true;
	do_preload_ = false;
}

pvs_database::
~pvs_database()
{
	if(visibility_grid_ != nullptr)
	{
		delete visibility_grid_;
	}
}

pvs_database* pvs_database::
get_instance()
{
	if(pvs_database::instance_ == nullptr)
	{
		pvs_database::instance_ = new pvs_database();
	}

	return pvs_database::instance_;
}

bool pvs_database::
load_pvs_from_file(const std::string& grid_file_path, const std::string& pvs_file_path, const bool& do_preload)
{
	std::lock_guard<std::mutex> lock(mutex_);

	do_preload_ = do_preload;
	visibility_grid_ = load_grid_from_file(grid_file_path);

	if(visibility_grid_ == nullptr)
	{
		// Loading grid file failed.
		return false;
	}
	
	if(do_preload_)
	{
		if(!visibility_grid_->load_visibility_from_file(pvs_file_path))
		{
			// Loading grid file failed.
			delete visibility_grid_;
			visibility_grid_ = nullptr;

			return false;
		}
	}

	pvs_file_path_ = pvs_file_path;

	return true;
}

grid* pvs_database::
load_grid_from_file(const std::string& grid_file_path) const
{
	grid* vis_grid = nullptr;

	std::fstream file_in;
	file_in.open(grid_file_path, std::ios::in);

	if(!file_in.is_open())
	{
		return nullptr;
	}

	// Read the grid file header to identify the grid type.
	std::string grid_type;
	file_in >> grid_type;

	file_in.close();

	vis_grid = create_grid_by_type(grid_type);

	if(vis_grid == nullptr || !vis_grid->load_grid_from_file(grid_file_path))
	{
		// Loading grid file failed.
		delete vis_grid;
		return nullptr;
	}

	return vis_grid;	
}

grid* pvs_database::
load_grid_from_file(const std::string& grid_file_path, const std::string& pvs_file_path) const
{
	grid* vis_grid = load_grid_from_file(grid_file_path);

	if(vis_grid == nullptr)
	{
		return nullptr;
	}
	
	if(!vis_grid->load_visibility_from_file(pvs_file_path))
	{
		// Loading grid file failed.
		delete vis_grid;
		return nullptr;
	}

	return vis_grid;
}

grid* pvs_database::
create_grid_by_type(const std::string& grid_type) const
{
	grid* output_grid = nullptr;

    if(grid_type == grid_regular::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_regular();
    }
    else if(grid_type == grid_regular_compressed::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_regular_compressed();
    }
    else if(grid_type == grid_octree::get_grid_identifier())
    {   
        output_grid = new lamure::pvs::grid_octree();
    }
    else if(grid_type == grid_octree_hierarchical::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_octree_hierarchical();
    }
    else if(grid_type == grid_octree_hierarchical_v2::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_octree_hierarchical_v2();
    }
    else if(grid_type == grid_octree_hierarchical_v3::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_octree_hierarchical_v3();
    }
    else if(grid_type == grid_irregular::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_irregular();
    }

    return output_grid;
}

grid* pvs_database::
create_grid_by_type(const std::string& grid_type, const size_t& num_cells_x, const size_t& num_cells_y, const size_t& num_cells_z, const double& bounds_size, const scm::math::vec3d& position_center, const std::vector<node_t>& ids) const
{
	grid* output_grid = nullptr;

	size_t max_num_cells = std::max(num_cells_x, std::max(num_cells_y, num_cells_z));

    if(grid_type == grid_regular::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_regular(max_num_cells, bounds_size, position_center, ids);
    }
    else if(grid_type == grid_regular_compressed::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_regular_compressed(max_num_cells, bounds_size, position_center, ids);
    }
    else if(grid_type == grid_octree::get_grid_identifier())
    {   
        output_grid = new lamure::pvs::grid_octree(max_num_cells, bounds_size, position_center, ids);
    }
    else if(grid_type == grid_octree_hierarchical::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_octree_hierarchical(max_num_cells, bounds_size, position_center, ids);
    }
    else if(grid_type == grid_octree_hierarchical_v2::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_octree_hierarchical_v2(max_num_cells, bounds_size, position_center, ids);
    }
    else if(grid_type == grid_octree_hierarchical_v3::get_grid_identifier())
    {
        output_grid = new lamure::pvs::grid_octree_hierarchical_v3(max_num_cells, bounds_size, position_center, ids);
    }
    else if(grid_type == grid_irregular::get_grid_identifier())
    {
    	output_grid = new lamure::pvs::grid_irregular(num_cells_x, num_cells_y, num_cells_z, bounds_size, position_center, ids);
    }

    return output_grid;
}

void pvs_database::
set_viewer_position(const scm::math::vec3d& position)
{
	std::lock_guard<std::mutex> lock(mutex_);

	if(activated_ && visibility_grid_ != nullptr)
	{
		if(position != position_viewer_)
		{
			position_viewer_ = position;
			size_t cell_index = 0;
			const view_cell* view_cell_at_position = visibility_grid_->get_cell_at_position(position, &cell_index);

			if(view_cell_at_position != viewer_cell_)
			{
				viewer_cell_ = view_cell_at_position;

				// If the view cell changed and the visibility data is not preloaded, it should be loaded now.
				if(!do_preload_)
				{
					visibility_grid_->load_cell_visibility_from_file(pvs_file_path_, cell_index);
				}
			}
		}
	}
}

bool pvs_database::
get_viewer_visibility(const model_t& model_id, const node_t node_id) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	if(!activated_ || viewer_cell_ == nullptr)
	{
		return true;
	}
	else
	{
		return viewer_cell_->get_visibility(model_id, node_id);
	}
}

void pvs_database::
activate(const bool& act)
{
	std::lock_guard<std::mutex> lock(mutex_);

	activated_ = act;
}

bool pvs_database::
is_activated() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	return activated_;
}

const grid* pvs_database::
get_visibility_grid() const
{
	std::lock_guard<std::mutex> lock(mutex_);

	return visibility_grid_;
}

void pvs_database::
clear_visibility_grid()
{
	std::lock_guard<std::mutex> lock(mutex_);

	viewer_cell_ = nullptr;

	delete visibility_grid_;
	visibility_grid_ = nullptr;
}

}
}

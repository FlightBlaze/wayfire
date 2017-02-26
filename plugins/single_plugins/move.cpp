#include <output.hpp>
#include <core.hpp>
#include <linux/input.h>

class wayfire_move : public wayfire_plugin_t {
    button_callback activate_binding;
    wayfire_view view;

    wl_fixed_t initial_x, initial_y;

    public:
        void init(weston_config *config) {
            debug << "loading move plugin" << std::endl;
            grab_interface->name = "move";
            grab_interface->compatAll = true;

            activate_binding = [=] (weston_pointer* ptr, uint32_t) {
                this->initiate(ptr);
            };

            using namespace std::placeholders;
            output->input->add_button(MODIFIER_ALT, BTN_LEFT, &activate_binding);
            grab_interface->callbacks.pointer.button =
                std::bind(std::mem_fn(&wayfire_move::button_pressed), this, _1, _2, _3);
            grab_interface->callbacks.pointer.motion =
                std::bind(std::mem_fn(&wayfire_move::pointer_motion), this, _1, _2);

            /* TODO: move_request, read binding from file and expo interoperability */
        }

        void initiate(weston_pointer *ptr) {
            debug << "initiate move" << std::endl;
            if (!ptr->focus)
                return;

            debug << "no focus" << std::endl;
            view = core->find_view(ptr->focus);
            if (!view)
                return;

            debug << "not found view" << std::endl;
            if (!output->input->activate_plugin(grab_interface))
                return;

            debug << "can't activate" << std::endl;
            weston_seat_break_desktop_grabs(ptr->seat);
            if (!grab_interface->grab())
                return;
            debug << "start grab" << std::endl;

            initial_x = wl_fixed_from_double(view->handle->geometry.x) - ptr->x;
            initial_y = wl_fixed_from_double(view->handle->geometry.y) - ptr->y;
        }

        void button_pressed(weston_pointer *ptr, uint32_t button, uint32_t state) {
            if (button != BTN_LEFT || state != WL_POINTER_BUTTON_STATE_RELEASED)
                return;

            grab_interface->ungrab();
            output->input->deactivate_plugin(grab_interface);
        }

        void pointer_motion(weston_pointer *ptr, weston_pointer_motion_event *ev) {
            view->move(wl_fixed_to_int(initial_x + ptr->x),
                    wl_fixed_to_int(initial_y + ptr->y));
        }
};

extern "C" {
    wayfire_plugin_t* newInstance() {
        return new wayfire_move();
    }
}


#include "openbox/action.h"
#include "openbox/focus.h"

static gboolean run_func(ObActionData *data, gpointer options);

void action_focustobottom_startup(void)
{
    action_register("FocusToBottom", OB_ACTION_DEFAULT_FILTER_SINGLE,
                    NULL, NULL, run_func);
}

/* Always return FALSE because its not interactive */
static gboolean run_func(ObActionData *data, gpointer options)
{
    if (data->client)
        focus_order_to_bottom(data->client);
    return FALSE;
}

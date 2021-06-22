package mirror.android.graphics.drawable;

import mirror.RefClass;
import mirror.RefMethod;

/**
 * @author Swift Gan
 * Create: 2019/4/23
 * Desc:
 */
public class LayerDrawable {
    public static Class<?> TYPE = RefClass.load(LayerDrawable.class, android.graphics.drawable.LayerDrawable.class);
    public static RefMethod<Boolean> isProjected;
}

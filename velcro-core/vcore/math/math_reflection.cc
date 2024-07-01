#include <vcore/math/math_reflection.h>
#include <vcore/math/uuid.h>
#include <vcore/math/crc.h>

#include <vcore/casting/lossy_cast.h>

#include <vcore/serialization/serialization_context.h>


namespace V
{
    void MathReflect(SerializeContext& context)
    {
        // aggregates
        //context.Class<Uuid>()->
        //    Serializer<UuidSerializer>();

        //Crc32::Reflect(context);
    }
}
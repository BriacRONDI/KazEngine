#pragma once

namespace Engine
{
    /**
     * Cette interface permet à une classe d'écouter les évènements en provenance du clavier
     */
    class IKeyboardListener
    {
        public:
            /**
             * Une touche est enfoncée
             */
            virtual void KeyDown(unsigned char Key) = 0;

            /**
             * Une touche est relachée
             */
            virtual void KeyUp(unsigned char Key) = 0;

            /**
             * Destructeur virtuel
             */
            virtual ~IKeyboardListener(){};
    };
}

# eeshelf
A smart store shelf with ARM mbed. This was created in 24 hrs on Make NTU event.

## The Problem
With more and more people in big cities, the waiting time in stores (e.g. 7-11) is getting longer and longer.
Sometimes we just want to grab a drink, but we often need to wait for a long time before checking out.
Thus we came up with the question: can we make the check out process faster?

## The Solution
We think that lots of time is wasted on calculating the quantity of items (e.g. scanning barcode, operating POS).
Our solution cuts this time by having the quantity calculated just after the items are taken from the shelves.
Each time a customer wants to get something from the shelf, the customer swipes a RFID card through the reader on the shelf. This card is each customer's identity.
After the customer finishes taking items, the customer presses a button, closing the gate of the shelf. The system calculates the quantity of items taken by measuring the distance from the wall to the end of item stack.
When the customer completes shopping, the cashier swipes the customer's RFID card. The computer at cashier then displays the total amount.

## Technologies
- ST NUCLEO-F429ZI at the shelf

We chose this board because of the variety of connection possibilities (many SPI, I2C channels), also Ethernet connectivity is great as this board should be a permanent installation

- ASUS Tinker Board at cashier

This board acts as the central unit of a store, receiving information from shelves and storing data to cloud.

- Microsoft Azure

We created a virtual machine running PostgreSQL, making each store can share their products information. Also customers can use Android App to access their basket data through API on this server.

## Difficulties
- Shelf ramp
- Door and its motors
- RFID reader

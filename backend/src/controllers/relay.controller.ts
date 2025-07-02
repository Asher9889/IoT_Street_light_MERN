import { NextFunction, Request, Response } from "express";
import { publishBulbStates } from "../utils";

let bulbStates: string[] = Array(10).fill("OFF");

async function updateBulb(req: Request, res: Response, next: NextFunction) {
    console.log("a connect hit: ", req.ip)
    const indexStr = req.body.index as string;
    const status = req.body.status as string;

    const index = parseInt(indexStr);

    if (isNaN(index) || index < 0 || index >= 10 || (status !== "ON" && status !== "OFF")) {
        return res.status(400).json({ success: false, msg: "Invalid index or status" });
    }

    bulbStates[index] = status;
    // publishBulbStates(bulbStates)   // 🟢 Publish via MQTT

    return res.json({ success: true, bulbStates });
}

async function getLatestCommand(req: Request, res: Response, next: NextFunction) {
    return res.status(200).json(bulbStates); // send only array to SIM900A
}


export { updateBulb, getLatestCommand }
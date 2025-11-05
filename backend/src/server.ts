import express from "express";
import cors from "cors";
import apiRoutes from "./routes"
import { globalErrorHandler } from "./utils";
import { config } from "./config";
import connectMongoDB from "./db/connectMongoDB";

const app = express();

connectMongoDB();

app.use(cors());
app.use(express.json({ limit: "10mb" }));
app.use(express.urlencoded({ extended: true }))


app.use("/api", apiRoutes)

app.use(globalErrorHandler as any)


app.listen(config.port, () => {
    console.log(`App started using port ${config.port}`)
})
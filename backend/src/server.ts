import express, { NextFunction, Request, Response } from "express";
import cors from "cors";
import apiRoutes from "./routes"
import { StatusCodes } from "http-status-codes";
import { ApiErrorResponse, globalErrorHandler } from "./utils";
const app = express();

app.use(cors());
app.use(express.json({ limit: "10mb" }));
app.use(express.urlencoded({ extended: true }))


app.use("/api", apiRoutes)

app.use(globalErrorHandler as any)


app.listen(5000, () => {
    console.log(`App started using port 5000`)
})
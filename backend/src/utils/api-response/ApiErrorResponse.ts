interface IApiError {
    status: boolean;
    statusCode: number;
    message: string;
    data?: any;
}

class ApiErrorResponse extends Error implements IApiError {
    status: boolean;
    statusCode: number;
    data?: any;
    name: string;

    constructor(statusCode: number, message: string, data?: any, stack?: string) {
        super(message);
        this.status = false;
        this.name = this.constructor.name;
        this.statusCode = statusCode;
        this.data = data || null;
        if (stack) {
            this.stack = stack;
        } else {
            Error.captureStackTrace(this, this.constructor);
        }
    }

    toJSON() {
        return {
            name: this.name,
            message: this.message,
            statusCode: this.statusCode,
            data: this.data,
            stack: this.stack,
        };
    }
}


export default ApiErrorResponse;